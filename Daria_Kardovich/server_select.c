#define _POSIX_C_SOURCE 200112L

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "./socket31"

static volatile sig_atomic_t stop_server;

static void stop_handler(int signal_number)
{
    (void)signal_number;
    stop_server = 1;
}

static int write_all(int fd, const char *buffer, size_t length)
{
    while (length > 0) {
        ssize_t written = write(fd, buffer, length);

        if (written < 0) {
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
    int max_fd;
    fd_set master_set;
    struct sockaddr_un address;
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = stop_handler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

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

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    max_fd = server_fd;

    printf("Server started. Ctrl+C stops server.\n");
    fflush(stdout);

    while (!stop_server) {
        fd_set ready_set = master_set;
        int ready = select(max_fd + 1, &ready_set, NULL, NULL, NULL);

        if (ready == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("select");
            break;
        }

        if (FD_ISSET(server_fd, &ready_set)) {
            int client_fd = accept(server_fd, NULL, NULL);

            if (client_fd == -1) {
                if (errno != EINTR) {
                    perror("accept");
                }
            } else if (client_fd >= FD_SETSIZE) {
                fprintf(stderr, "Too many clients\n");
                close(client_fd);
            } else {
                FD_SET(client_fd, &master_set);

                if (client_fd > max_fd) {
                    max_fd = client_fd;
                }

                printf("Client connected.\n");
                fflush(stdout);
            }
        }

        for (int fd = 0; fd <= max_fd; ++fd) {
            char buffer[1024];
            ssize_t count;

            if (fd == server_fd || !FD_ISSET(fd, &ready_set)) {
                continue;
            }

            count = read(fd, buffer, sizeof(buffer));

            if (count == 0) {
                close(fd);
                FD_CLR(fd, &master_set);
                printf("Client disconnected.\n");
                fflush(stdout);
                continue;
            }

            if (count == -1) {
                if (errno != EINTR) {
                    perror("read");
                }

                continue;
            }

            for (ssize_t i = 0; i < count; ++i) {
                buffer[i] = (char)toupper((unsigned char)buffer[i]);
            }

            if (write_all(STDOUT_FILENO, buffer, (size_t)count) == -1) {
                perror("write");
                stop_server = 1;
                break;
            }
        }
    }

    for (int fd = 0; fd <= max_fd; ++fd) {
        if (FD_ISSET(fd, &master_set)) {
            close(fd);
        }
    }

    unlink(SOCKET_PATH);
    return 0;
}
