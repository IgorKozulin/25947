
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <aio.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 100
#define AIO_SIGNAL SIGUSR1

struct client {
    int fd;
    int id;
    struct aiocb cb;
    char byte;
    int pending;
};

struct client clients[MAX_CLIENTS];

volatile sig_atomic_t running = 1;
volatile sig_atomic_t aio_ready = 0;

void stop_handler(int sig){
    running = 0;
}

void aio_handler(int sig){
    aio_ready = 1;
}

int arm_read(struct client *c){
    memset(&c->cb, 0, sizeof(c->cb));

    c->cb.aio_fildes = c->fd;
    c->cb.aio_buf = &c->byte;
    c->cb.aio_nbytes = 1;
    c->cb.aio_offset = 0;
    c->cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    c->cb.aio_sigevent.sigev_signo = AIO_SIGNAL;

    if (aio_read(&c->cb) == -1){
        return -1;
    }

    c->pending = 1;
    return 0;
}

int main(){
    int server_fd;
    int next_id = 1;
    int total = 0;
    struct sockaddr_un address;
    struct sigaction sa;
    struct timespec delay = {0, 10000000L};

    for (int i = 0; i < MAX_CLIENTS; i++){
        clients[i].fd = -1;
        clients[i].pending = 0;
    }

    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = stop_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = aio_handler;
    sigaction(AIO_SIGNAL, &sa, NULL);

    unlink(SOCKET_PATH);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd == -1){
        perror("socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) == -1){
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1){
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    int flags = fcntl(server_fd, F_GETFL, 0);

    if (flags == -1 ||
        fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1){
        perror("fcntl");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("[SERVER] AIO server started on %s\n", SOCKET_PATH);
    printf("[SERVER] Press Ctrl+C to stop.\n");
    fflush(stdout);

    while (running){

        while (running){
            int client_fd = accept(server_fd, NULL, NULL);

            if (client_fd == -1){
                if (errno == EINTR){
                    continue;
                }

                if (errno != EAGAIN && errno != EWOULDBLOCK){
                    perror("accept");
                }
                break;
            }

            int slot = -1;

            for (int i = 0; i < MAX_CLIENTS; i++){
                if (clients[i].fd == -1){
                    slot = i;
                    break;
                }
            }

            if (slot == -1){
                close(client_fd);
                continue;
            }

            int cflags = fcntl(client_fd, F_GETFL, 0);

            if (cflags == -1 ||
                fcntl(client_fd, F_SETFL,
                      cflags & ~O_NONBLOCK) == -1){
                close(client_fd);
                continue;
            }

            struct client *c = &clients[slot];

            c->fd = client_fd;
            c->id = next_id++;
            c->pending = 0;

            if (arm_read(c) == -1){
                perror("aio_read");
                close(c->fd);
                c->fd = -1;
                continue;
            }

            total++;

            printf("[SERVER] Client %d connected. Total: %d\n",
                   c->id, total);
            fflush(stdout);
        }

        for (int i = 0; i < MAX_CLIENTS; i++){
            struct client *c = &clients[i];

            if (c->fd == -1 || !c->pending){
                continue;
            }

            int err = aio_error(&c->cb);

            if (err == EINPROGRESS || err == EAGAIN){
                continue;
            }

            ssize_t n = aio_return(&c->cb);
            c->pending = 0;

            if (err == 0 && n > 0){
                char ch = toupper((unsigned char)c->byte);
                write(STDOUT_FILENO, &ch, 1);

                if (arm_read(c) == 0){
                    continue;
                }

                perror("aio_read");
            }
            else if (err != 0 && err != ECANCELED){
                fprintf(stderr, "AIO error: %s\n", strerror(err));
            }

            printf("\n[SERVER] Client %d disconnected.\n", c->id);

            close(c->fd);
            c->fd = -1;
            total--;

            printf("[SERVER] Remaining clients: %d\n", total);
            fflush(stdout);
        }

        aio_ready = 0;
        nanosleep(&delay, NULL);
    }

    for (int i = 0; i < MAX_CLIENTS; i++){
        if (clients[i].fd != -1){
            if (clients[i].pending){
                aio_cancel(clients[i].fd, &clients[i].cb);

                while (aio_error(&clients[i].cb) == EINPROGRESS){
                    nanosleep(&delay, NULL);
                }

                aio_return(&clients[i].cb);
            }

            close(clients[i].fd);
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);

    printf("\n[SERVER] Finished.\n");

    return 0;
}

