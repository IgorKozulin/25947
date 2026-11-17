#define __EXTENSIONS__ 1
#define _POSIX_C_SOURCE 200112L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "./socket32"
#define MAX_CLIENTS 64

static volatile sig_atomic_t event_happened = 1;
static volatile sig_atomic_t server_running = 1;

static void sigio_handler(int signal_number)
{
    (void)signal_number;
    event_happened = 1;
}

static void sigint_handler(int signal_number)
{
    (void)signal_number;
    server_running = 0;
}

static int enable_async_io(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }

    if (fcntl(fd, F_SETOWN, getpid()) == -1) {
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | FNDELAY | FASYNC) == -1) {
        return -1;
    }

    return 0;
}

static void close_client(int clients[], int index)
{
    close(clients[index]);
    clients[index] = -1;
}

static void accept_new_clients(int server_fd, int clients[])
{
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return;
            }

            perror("accept");
            return;
        }

        if (enable_async_io(client_fd) == -1) {
            perror("enable_async_io for client");
            close(client_fd);
            continue;
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == -1) {
                clients[i] = client_fd;
                printf("Client connected.\n");
                fflush(stdout);
                client_fd = -1;
                break;
            }
        }

        if (client_fd != -1) {
            printf("Too many clients. Connection rejected.\n");
            close(client_fd);
        }
    }
}

static void read_clients(int clients[])
{
    char buffer[1024];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == -1) {
            continue;
        }

        while (1) {
            ssize_t count = read(clients[i], buffer, sizeof(buffer));

            if (count > 0) {
                for (ssize_t j = 0; j < count; j++) {
                    buffer[j] = (char)toupper((unsigned char)buffer[j]);
                }

                if (write(STDOUT_FILENO, buffer, (size_t)count) == -1) {
                    perror("write");
                }

                continue;
            }

            if (count == 0) {
                printf("Client disconnected.\n");
                close_client(clients, i);
                break;
            }

            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }

            perror("read");
            close_client(clients, i);
            break;
        }
    }
}

int main(void)
{
    int server_fd;
    int clients[MAX_CLIENTS];
    struct sockaddr_un address;
    struct sigaction action_io;
    struct sigaction action_int;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
    }

    memset(&action_io, 0, sizeof(action_io));
    action_io.sa_handler = sigio_handler;
    sigemptyset(&action_io.sa_mask);
    action_io.sa_flags = 0;

    if (sigaction(SIGIO, &action_io, NULL) == -1) {
        perror("sigaction SIGIO");
        return 1;
    }

    memset(&action_int, 0, sizeof(action_int));
    action_int.sa_handler = sigint_handler;
    sigemptyset(&action_int.sa_mask);
    action_int.sa_flags = 0;

    if (sigaction(SIGINT, &action_int, NULL) == -1) {
        perror("sigaction SIGINT");
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

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    if (enable_async_io(server_fd) == -1) {
        perror("enable_async_io for server");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("Server started. Socket: %s\n", SOCKET_PATH);
    printf("Start one or more clients. Ctrl+C stops server.\n");
    fflush(stdout);

    while (server_running) {
        if (event_happened) {
            event_happened = 0;
            accept_new_clients(server_fd, clients);
            read_clients(clients);
        }

        pause();
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) {
            close(clients[i]);
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);

    printf("\nServer finished.\n");
    return 0;
}
