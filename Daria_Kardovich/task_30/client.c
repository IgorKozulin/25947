#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "./socket"

static int write_all(int fd, const char *buffer, size_t length)
{
    while (length > 0) {
        ssize_t written = write(fd, buffer, length);

        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (written == 0) {
            return -1;
        }

        buffer += written;
        length -= (size_t)written;
    }

    return 0;
}

int main(void)
{
    int socket_fd;
    struct sockaddr_un address;
    char buffer[1024];

    signal(SIGPIPE, SIG_IGN);

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("connect");
        close(socket_fd);
        return 1;
    }

    printf("Connected to server.\n");
    printf("Enter text. Empty line or Ctrl+D finishes input.\n");

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (buffer[0] == '\n') {
            break;
        }

        if (write_all(socket_fd, buffer, strlen(buffer)) == -1) {
            perror("write");
            close(socket_fd);
            return 1;
        }
    }

    close(socket_fd);

    printf("Connection closed.\n");
    return 0;
}
