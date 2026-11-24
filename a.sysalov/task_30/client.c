#define _POSIX_C_SOURCE 200809L
#include "socket_io.h"

int main(int argc, char *argv[])
{
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [socket_path]\n", argv[0]);
        return 1;
    }
    const char *path = argc == 2 ? argv[1] : "./socket";
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

    unsigned char buffer[1024];
    int status = 0;
    while (1) {
        ssize_t length = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (length == -1 && errno == EINTR) continue;
        if (length == 0) break;
        if (length == -1) {
            perror("read");
            status = 1;
            break;
        }
        if (write_all(fd, buffer, (size_t)length) == -1) {
            perror("write");
            status = 1;
            break;
        }
    }
    close(fd);
    return status;
}
