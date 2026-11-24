#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 16

static volatile sig_atomic_t running = 1;

static void stop_server(int sig)
{
    (void)sig;
    running = 0;
}

static int nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int read_client(int fd)
{
    char buffer[4096];
    ssize_t count = read(fd, buffer, sizeof(buffer));

    if (count == -1 &&
        (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 1;
    }
    if (count <= 0) {
        if (count == -1) {
            perror("read client");
        }
        return 0;
    }

    for (ssize_t i = 0; i < count; i++) {
        buffer[i] = toupper((unsigned char)buffer[i]);
    }

    if (fwrite(buffer, 1, (size_t)count, stdout) != (size_t)count ||
        fflush(stdout) == EOF) {
        perror("stdout");
        return -1;
    }
    return 1;
}

int main(void)
{
    struct sigaction action = {0};
    struct sockaddr_un address = {0};
    struct pollfd fds[MAX_CLIENTS + 1];
    int error = 0;

    action.sa_handler = stop_server;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) == -1 ||
        sigaction(SIGTERM, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

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
    if (listen(server_fd, MAX_CLIENTS) == -1 || nonblocking(server_fd) == -1) {
        perror("listen/fcntl");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    for (int i = 0; i <= MAX_CLIENTS; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }
    fds[0].fd = server_fd;

    fprintf(stderr, "Server: waiting on %s. Ctrl+C to stop.\n", SOCKET_PATH);

    while (running) {
        int ready = poll(fds, MAX_CLIENTS + 1, 1000);

        if (ready == -1 && errno == EINTR) {
            continue;
        }
        if (ready == -1) {
            perror("poll");
            error = 1;
            break;
        }
        if (!running) {
            break;
        }

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fputs("Listening socket error\n", stderr);
            error = 1;
            break;
        }

        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, NULL, NULL);

            if (client_fd == -1) {
                if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("accept");
                }
            } else if (nonblocking(client_fd) == -1) {
                perror("fcntl client");
                close(client_fd);
            } else {
                int slot = 1;
                while (slot <= MAX_CLIENTS && fds[slot].fd != -1) {
                    slot++;
                }

                if (slot > MAX_CLIENTS) {
                    fputs("Too many clients\n", stderr);
                    close(client_fd);
                } else {
                    fds[slot].fd = client_fd;
                    fds[slot].revents = 0;
                    fprintf(stderr, "Client connected: fd=%d\n", client_fd);
                }
            }
        }

        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (fds[i].fd == -1 || fds[i].revents == 0) {
                continue;
            }

            int result = (fds[i].revents & POLLNVAL) ? 0 : read_client(fds[i].fd);

            if (result == -1) {
                error = 1;
                break;
            }
            if (result == 0) {
                fprintf(stderr, "Client disconnected: fd=%d\n", fds[i].fd);
                close(fds[i].fd);
                fds[i].fd = -1;
            }
        }

        if (error) {
            break;
        }
    }

    for (int i = 0; i <= MAX_CLIENTS; i++) {
        if (fds[i].fd != -1) {
            close(fds[i].fd);
        }
    }

    if (unlink(SOCKET_PATH) == -1) {
        perror("unlink");
        error = 1;
    }
    return error;
}
