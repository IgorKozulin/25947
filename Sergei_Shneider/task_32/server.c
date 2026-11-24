#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <aio.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 16

typedef struct {
    int fd;
    int active;
    char byte;
    struct aiocb cb;
} Client;

static Client clients[MAX_CLIENTS];

static int start_read(Client *client)
{
    memset(&client->cb, 0, sizeof(client->cb));
    client->cb.aio_fildes = client->fd;
    client->cb.aio_buf = &client->byte;
    client->cb.aio_nbytes = 1;
    client->cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    client->cb.aio_sigevent.sigev_signo = SIGUSR1;

    if (aio_read(&client->cb) == -1) {
        perror("aio_read");
        return -1;
    }

    client->active = 1;
    return 0;
}

static void close_client(Client *client)
{
    if (client->fd == -1) {
        return;
    }

    if (client->active) {
        shutdown(client->fd, SHUT_RDWR);
        aio_cancel(client->fd, &client->cb);

        const struct aiocb *list[] = {&client->cb};
        while (aio_error(&client->cb) == EINPROGRESS) {
            aio_suspend(list, 1, NULL);
        }

        aio_return(&client->cb);
        client->active = 0;
    }

    close(client->fd);
    client->fd = -1;
}

int main(void)
{
    sigset_t signals;
    struct sockaddr_un address = {0};
    int error = 0;
    int running = 1;

    sigemptyset(&signals);
    sigaddset(&signals, SIGUSR1);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
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

    int flags = fcntl(server_fd, F_GETFL, 0);
    if (listen(server_fd, MAX_CLIENTS) == -1 || flags == -1 ||
        fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("listen/fcntl");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    fprintf(stderr, "AIO server: waiting on %s. Ctrl+C to stop.\n", SOCKET_PATH);

    while (running) {
        for (int attempt = 0; attempt < MAX_CLIENTS; attempt++) {
            int fd = accept(server_fd, NULL, NULL);

            if (fd == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                    perror("accept");
                    error = 1;
                }
                break;
            }

            int slot = 0;
            while (slot < MAX_CLIENTS && clients[slot].fd != -1) {
                slot++;
            }

            if (slot == MAX_CLIENTS) {
                fputs("Too many clients\n", stderr);
                close(fd);
                continue;
            }

            flags = fcntl(fd, F_GETFL, 0);
            if (flags == -1 || fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
                perror("fcntl client");
                close(fd);
                continue;
            }

            clients[slot].fd = fd;

            if (start_read(&clients[slot]) == -1) {
                close_client(&clients[slot]);
            } else {
                fprintf(stderr, "Client connected: fd=%d\n", fd);
            }
        }

        if (error) {
            break;
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            Client *client = &clients[i];

            if (client->fd == -1 || !client->active) {
                continue;
            }

            int status = aio_error(&client->cb);
            if (status == EINPROGRESS) {
                continue;
            }

            ssize_t count = aio_return(&client->cb);
            client->active = 0;

            if (status != 0 || count <= 0) {
                if (status != 0) {
                    fprintf(stderr, "AIO: %s\n",
                            strerror(status == -1 ? errno : status));
                }

                fprintf(stderr, "Client disconnected: fd=%d\n", client->fd);
                close_client(client);
                continue;
            }

            if (putchar(toupper((unsigned char)client->byte)) == EOF) {
                perror("stdout");
                error = 1;
                break;
            }

            if (start_read(client) == -1) {
                close_client(client);
            }
        }

        if (fflush(stdout) == EOF) {
            perror("stdout");
            error = 1;
        }
        if (error) {
            break;
        }

        struct timespec timeout = {0, 10000000};
        int sig = sigtimedwait(&signals, NULL, &timeout);

        if (sig == SIGINT || sig == SIGTERM) {
            running = 0;
        } else if (sig == -1 && errno != EAGAIN && errno != EINTR) {
            perror("sigtimedwait");
            error = 1;
            break;
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        close_client(&clients[i]);
    }

    close(server_fd);

    if (unlink(SOCKET_PATH) == -1) {
        perror("unlink");
        error = 1;
    }

    return error;
}
