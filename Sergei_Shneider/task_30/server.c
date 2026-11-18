#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(void)
{
    struct sockaddr_un address = {0};
    char buffer[4096];
    int error = 0;
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    if (unlink(SOCKET_PATH) == -1 && errno != ENOENT) {
        perror("unlink");
        close(server_fd);
        return 1;
    }

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    puts("Waiting for client on ./socket...");
    fflush(stdout);

    int client_fd;
    do {
        client_fd = accept(server_fd, NULL, NULL);
    } while (client_fd == -1 && errno == EINTR);

    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    while (1) {
        ssize_t count = read(client_fd, buffer, sizeof(buffer));

        if (count == -1 && errno == EINTR) {
            continue;
        }

        if (count <= 0) {
            if (count == -1) {
                perror("read");
                error = 1;
            }
            break;
        }

        for (ssize_t i = 0; i < count; i++) {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }

        if (fwrite(buffer, 1, count, stdout) != (size_t)count ||
            fflush(stdout) == EOF) {
            perror("stdout");
            error = 1;
            break;
        }
    }

    close(client_fd);
    close(server_fd);

    if (unlink(SOCKET_PATH) == -1) {
        perror("unlink");
        error = 1;
    }

    return error;
}
