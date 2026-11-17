#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

static int write_all(int fd, const char *buffer, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t count = write(fd, buffer + sent, length - sent);

        if (count == -1 && errno == EINTR) {
            continue;
        }

        if (count <= 0) {
            if (count == 0) {
                errno = EIO;
            }
            return -1;
        }

        sent += count;
    }

    return 0;
}

int main(void)
{
    struct sockaddr_un address = {0};
    char buffer[4096];
    int error = 0;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd == -1) {
        perror("socket");
        return 1;
    }

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }

    while (1) {
        ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));

        if (count == -1 && errno == EINTR) {
            continue;
        }

        if (count <= 0) {
            if (count == -1) {
                perror("stdin");
                error = 1;
            }
            break;
        }

        if (write_all(fd, buffer, count) == -1) {
            perror("write");
            error = 1;
            break;
        }
    }

    close(fd);
    return error;
}
