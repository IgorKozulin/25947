#define _POSIX_C_SOURCE 200809L
#include "socket_io.h"

int main(int argc, char *argv[])
{
    const char *name = NULL;
    int option;
    while ((option = getopt(argc, argv, "n:")) != -1) {
        if (option != 'n') return 1;
        name = optarg;
    }
    if (argc - optind > 1 || (name && strpbrk(name, "\r\n"))) {
        fprintf(stderr, "Usage: %s [-n name] [socket_path]\n", argv[0]);
        return 1;
    }
    const char *path = optind < argc ? argv[optind] : "./socket";
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

    char *line = NULL;
    size_t capacity = 0;
    int status = 0;
    ssize_t length;
    while ((length = getline(&line, &capacity, stdin)) != -1) {
        if (name && (write_all(fd, (const unsigned char *)"[", 1) == -1
                || write_all(fd, (const unsigned char *)name, strlen(name)) == -1
                || write_all(fd, (const unsigned char *)"] ", 2) == -1)) {
            perror("write");
            status = 1;
            break;
        }
        if (write_all(fd, (const unsigned char *)line, (size_t)length) == -1) {
            perror("write");
            status = 1;
            break;
        }
    }
    if (ferror(stdin)) {
        perror("getline");
        status = 1;
    }
    free(line);
    close(fd);
    return status;
}
