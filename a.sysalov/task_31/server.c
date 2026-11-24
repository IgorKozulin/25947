#define _POSIX_C_SOURCE 200809L
#include <poll.h>
#include <stdint.h>
#include "socket_io.h"

#define MAX_CLIENTS 64

struct ClientLine {
    unsigned char *text;
    size_t length;
    size_t capacity;
};

int print_line(int fd, struct ClientLine *line)
{
    if (line->length == 0) return 0;
    char prefix[64];
    int size = snprintf(prefix, sizeof(prefix), "[Client %d]: ", fd);
    if (write_all(STDOUT_FILENO, (const unsigned char *)prefix, (size_t)size) == -1
            || print_upper(line->text, line->length) == -1) return -1;
    if (line->text[line->length - 1] != '\n'
            && write_all(STDOUT_FILENO, (const unsigned char *)"\n", 1) == -1) return -1;
    line->length = 0;
    return 0;
}

int receive_lines(int fd, struct ClientLine *line, const unsigned char *text, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        if (line->length == line->capacity) {
            if (line->capacity > SIZE_MAX / 2) {
                errno = ENOMEM;
                return -1;
            }
            size_t capacity = line->capacity ? line->capacity * 2 : 1024;
            unsigned char *buffer = realloc(line->text, capacity);
            if (buffer == NULL) return -1;
            line->text = buffer;
            line->capacity = capacity;
        }
        line->text[line->length++] = text[i];
        if (text[i] == '\n' && print_line(fd, line) == -1) return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [socket_path]\n", argv[0]);
        return 1;
    }
    const char *path = argc == 2 ? argv[1] : "./socket";
    if (setup_signals(1) == -1) {
        perror("signal setup");
        return 1;
    }
    int server = open_server(path);
    if (server == -1) {
        perror("server socket");
        return 1;
    }
    fprintf(stderr, "Server listening on %s. Ctrl+C to stop.\n", path);

    struct pollfd descriptors[MAX_CLIENTS + 1];
    struct ClientLine lines[MAX_CLIENTS + 1] = {0};
    for (int i = 0; i <= MAX_CLIENTS; i++) {
        descriptors[i].fd = -1;
        descriptors[i].events = POLLIN;
        descriptors[i].revents = 0;
    }
    descriptors[0].fd = server;
    int status = 0;
    int client_count = 0;
    while (1) {
        if (poll(descriptors, MAX_CLIENTS + 1, -1) == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            status = 1;
            break;
        }
        if (descriptors[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "Listening socket failed.\n");
            status = 1;
            break;
        }
        if (descriptors[0].revents & POLLIN) {
            int client = accept(server, NULL, NULL);
            if (client == -1) {
                if (errno == EINTR) continue;
                perror("accept");
                status = 1;
                break;
            }
            int slot = 1;
            while (slot <= MAX_CLIENTS && descriptors[slot].fd != -1) slot++;
            if (slot <= MAX_CLIENTS) {
                descriptors[slot].fd = client;
                descriptors[slot].revents = 0;
                client_count++;
                fprintf(stderr, "New client connected (fd=%d). Total: %d\n",
                        client, client_count);
            } else {
                fprintf(stderr, "Too many clients.\n");
                close(client);
            }
        }
        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (descriptors[i].fd == -1 || descriptors[i].revents == 0) continue;
            if (descriptors[i].revents & POLLNVAL) {
                if (print_line(descriptors[i].fd, &lines[i]) == -1) {
                    perror("stdout");
                    status = 1;
                    goto cleanup;
                }
                free(lines[i].text);
                memset(&lines[i], 0, sizeof(lines[i]));
                fprintf(stderr, "Client %d disconnected.\n", descriptors[i].fd);
                close(descriptors[i].fd);
                descriptors[i].fd = -1;
                client_count--;
                fprintf(stderr, "Clients remaining: %d\n", client_count);
                continue;
            }
            unsigned char buffer[1024];
            ssize_t length = read(descriptors[i].fd, buffer, sizeof(buffer));
            if (length == -1 && errno == EINTR) continue;
            if (length <= 0) {
                if (length == -1) perror("read");
                if (print_line(descriptors[i].fd, &lines[i]) == -1) {
                    perror("stdout");
                    status = 1;
                    goto cleanup;
                }
                free(lines[i].text);
                memset(&lines[i], 0, sizeof(lines[i]));
                fprintf(stderr, "Client %d disconnected.\n", descriptors[i].fd);
                close(descriptors[i].fd);
                descriptors[i].fd = -1;
                client_count--;
                fprintf(stderr, "Clients remaining: %d\n", client_count);
                continue;
            }
            if (receive_lines(descriptors[i].fd, &lines[i], buffer, (size_t)length) == -1) {
                perror("client output");
                status = 1;
                goto cleanup;
            }
        }
    }
cleanup:
    for (int i = 0; i <= MAX_CLIENTS; i++) {
        if (descriptors[i].fd != -1) close(descriptors[i].fd);
        free(lines[i].text);
    }
    return status;
}
