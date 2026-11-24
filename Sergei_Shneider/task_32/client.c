#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
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
        sent += (size_t)count;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *name = argc > 1 ? argv[1] : "client";
    const char *messages[] = {
        "hello from client\n",
        "this is a test message\n",
        "asynchronous io is awesome\n"
    };
    struct sockaddr_un address = {0};
    char buffer[512];
    int error = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [name]\n", argv[0]);
        return 1;
    }
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

    for (int i = 0; i < 3; i++) {
        int length = snprintf(buffer, sizeof(buffer), "[%s] %s", name, messages[i]);

        if (length < 0 || (size_t)length >= sizeof(buffer)) {
            fputs("Client name is too long\n", stderr);
            error = 1;
            break;
        }
        if (write_all(fd, buffer, (size_t)length) == -1) {
            perror("write");
            error = 1;
            break;
        }

        printf("Sent: %s", buffer);
        fflush(stdout);
        sleep(1);
    }

    close(fd);
    return error;
}
