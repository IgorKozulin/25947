#define _POSIX_C_SOURCE 200112L

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
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
    int server_fd;
    int client_fd;
    struct sockaddr_un address;
    char buffer[1024];

    unlink(SOCKET_PATH);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("Server started. Waiting for a client on %s\n", SOCKET_PATH);
    fflush(stdout);

    client_fd = accept(server_fd, NULL, NULL);

    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("Client connected.\n");
    fflush(stdout);

    while (1) {
        ssize_t count = read(client_fd, buffer, sizeof(buffer));

        if (count == 0) {
            break;
        }

        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("read");
            close(client_fd);
            close(server_fd);
            unlink(SOCKET_PATH);
            return 1;
        }

        for (ssize_t i = 0; i < count; i++) {
            buffer[i] = (char)toupper((unsigned char)buffer[i]);
        }

        if (write_all(STDOUT_FILENO, buffer, (size_t)count) == -1) {
            perror("write");
            close(client_fd);
            close(server_fd);
            unlink(SOCKET_PATH);
            return 1;
        }
    }

    printf("Client disconnected. Server finished.\n");

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return 0;
}
