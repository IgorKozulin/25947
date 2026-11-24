#ifndef SOCKET_IO_H
#define SOCKET_IO_H

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static const char *socket_path = NULL;

static inline void remove_socket(void)
{
    if (socket_path != NULL) unlink(socket_path);
}

static inline void stop_server(int signal_number)
{
    (void)signal_number;
    remove_socket();
    _exit(0);
}

static inline int setup_signals(int server)
{
    struct sigaction action = {0};
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGPIPE, &action, NULL) == -1) return -1;
    if (server) {
        action.sa_handler = stop_server;
        if (sigaction(SIGINT, &action, NULL) == -1
                || sigaction(SIGTERM, &action, NULL) == -1) return -1;
        if (atexit(remove_socket) != 0) {
            errno = ENOMEM;
            return -1;
        }
    }
    return 0;
}

static inline int make_address(struct sockaddr_un *address, const char *path)
{
    memset(address, 0, sizeof(*address));
    address->sun_family = AF_UNIX;
    if (strlen(path) >= sizeof(address->sun_path)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(address->sun_path, path);
    return 0;
}

static inline int open_server(const char *path)
{
    struct sockaddr_un address;
    if (make_address(&address, path) == -1) return -1;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) return -1;
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
    socket_path = path;
    if (listen(fd, 16) == -1) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
    return fd;
}

static inline int write_all(int fd, const unsigned char *buffer, size_t length)
{
    size_t total = 0;
    while (total < length) {
        ssize_t written = write(fd, buffer + total, length - total);
        if (written == -1 && errno == EINTR) continue;
        if (written <= 0) {
            if (written == 0) errno = EIO;
            return -1;
        }
        total += (size_t)written;
    }
    return 0;
}

static inline int print_upper(unsigned char *buffer, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        buffer[i] = (unsigned char)toupper(buffer[i]);
    }
    return write_all(STDOUT_FILENO, buffer, length);
}

#endif
