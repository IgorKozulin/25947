#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include "socket_io.h"

int main(int argc, char *argv[])
{
    if (argc > 3) {
        fprintf(stderr, "Usage: %s [name [socket_path]]\n", argv[0]);
        return 1;
    }
    const char *name = argc > 1 ? argv[1] : "client";
    const char *path = argc > 2 ? argv[2] : "./socket";
    if (strpbrk(name, "\r\n") != NULL) {
        fprintf(stderr, "Client name must not contain newlines.\n");
        return 1;
    }
    if (setup_signals(0) == -1) {
        perror("sigaction");
        return 1;
    }
    struct sockaddr_un address;
    if (make_address(&address, path) == -1) {
        perror("socket path");
        return 1;
    }
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }
    const char *messages[] = {
        "hello from client\n",
        "this is a test message\n",
        "asynchronous io is awesome\n"
    };
    int status = 0;
    for (size_t i = 0; i < sizeof(messages) / sizeof(messages[0]); i++) {
        if (write_all(fd, (const unsigned char *)"[", 1) == -1
                || write_all(fd, (const unsigned char *)name, strlen(name)) == -1
                || write_all(fd, (const unsigned char *)"] ", 2) == -1
                || write_all(fd, (const unsigned char *)messages[i], strlen(messages[i])) == -1) {
            perror("write");
            status = 1;
            break;
        }
        if (i < 2) {
            struct timespec delay = {0, 500000000};
            while (nanosleep(&delay, &delay) == -1 && errno == EINTR) {}
        }
    }
    close(fd);
    return status;
}
