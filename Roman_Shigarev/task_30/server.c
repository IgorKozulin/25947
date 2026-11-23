#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "./socket"

static int print_upper(int fd)
{
    unsigned char buffer[1024];

    for (;;) {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n == 0)
            return 0;
        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            return 1;
        }

        for (ssize_t i = 0; i < n; i++)
            buffer[i] = (unsigned char)toupper(buffer[i]);

        ssize_t sent = 0;
        while (sent < n) {
            ssize_t m = write(STDOUT_FILENO, buffer + sent,
                              (size_t)(n - sent));
            if (m == -1) {
                if (errno == EINTR)
                    continue;
                perror("write");
                return 1;
            }
            sent += m;
        }
    }
}

int main(void)
{
    struct stat old_socket;
    if (lstat(SOCKET_PATH, &old_socket) == 0) {
        if (!S_ISSOCK(old_socket.st_mode)) {
            fputs(SOCKET_PATH " is not a socket\n", stderr);
            return 1;
        }
        if (unlink(SOCKET_PATH) == -1) {
            perror("unlink");
            return 1;
        }
    } else if (errno != ENOENT) {
        perror("lstat");
        return 1;
    }

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un address = {0};
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    int result = 1;

    if (listen(server_fd, 5) == -1) {
        perror("listen");
    } else {
        fputs("Waiting for a client on " SOCKET_PATH "...\n", stderr);

        int client_fd;
        do {
            client_fd = accept(server_fd, NULL, NULL);
        } while (client_fd == -1 && errno == EINTR);

        if (client_fd == -1) {
            perror("accept");
        } else {
            result = print_upper(client_fd);
            close(client_fd);
        }
    }

    close(server_fd);
    if (unlink(SOCKET_PATH) == -1) {
        perror("unlink");
        return 1;
    }

    return result;
}
