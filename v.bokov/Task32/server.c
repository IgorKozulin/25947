#include <stdio.h>
#include <stdlib.h>
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

struct Client
{
    int fd;
    int id;
    char byte;
    struct aiocb cb;
    int pending;
};

struct Client clients[MAX_CLIENTS];

volatile sig_atomic_t running = 1;

void stop_server(int sig)
{
    (void)sig;
    running = 0;
}

int arm_read(struct Client *c)
{
    memset(&c->cb, 0, sizeof(c->cb));

    c->cb.aio_fildes = c->fd;
    c->cb.aio_buf = &c->byte;
    c->cb.aio_nbytes = 1;
    c->cb.aio_offset = 0;
    c->cb.aio_sigevent.sigev_notify = SIGEV_NONE;

    if (aio_read(&c->cb) == -1)
    {
        perror("aio_read");
        return -1;
    }

    c->pending = 1;
    return 0;
}

void close_client(struct Client *c, int *count)
{
    printf("\nClient %d disconnected.\n", c->id);

    close(c->fd);
    c->fd = -1;
    c->pending = 0;
    (*count)--;

    printf("Clients remaining: %d\n", *count);
    fflush(stdout);
}

int main(void)
{
    int server_fd;
    struct sockaddr_un addr;
    struct sigaction sa;
    int count = 0;
    int next_id = 1;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = stop_server;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = -1;
        clients[i].pending = 0;
    }

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&addr,
             sizeof(addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1)
    {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    int flags = fcntl(server_fd, F_GETFL, 0);

    if (flags == -1 ||
        fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("AIO server started on %s\n", SOCKET_PATH);
    printf("Press Ctrl+C to stop.\n");
    fflush(stdout);

    while (running)
    {
        while (running)
        {
            int fd = accept(server_fd, NULL, NULL);

            if (fd == -1)
            {
                if (errno == EINTR)
                    continue;

                if (errno != EAGAIN && errno != EWOULDBLOCK)
                    perror("accept");

                break;
            }

            int slot = -1;

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].fd == -1)
                {
                    slot = i;
                    break;
                }
            }

            if (slot == -1)
            {
                printf("Too many clients!\n");
                close(fd);
                continue;
            }

            int cflags = fcntl(fd, F_GETFL, 0);

            if (cflags == -1 ||
                fcntl(fd, F_SETFL,
                      cflags & ~O_NONBLOCK) == -1)
            {
                perror("fcntl client");
                close(fd);
                continue;
            }

            clients[slot].fd = fd;
            clients[slot].id = next_id++;
            clients[slot].pending = 0;

            if (arm_read(&clients[slot]) == -1)
            {
                close(fd);
                clients[slot].fd = -1;
                continue;
            }

            count++;

            printf("Client %d connected (slot=%d, fd=%d). Total: %d\n",
                   clients[slot].id, slot, fd, count);
            fflush(stdout);
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            struct Client *c = &clients[i];

            if (c->fd == -1 || !c->pending)
                continue;

            int err = aio_error(&c->cb);

            if (err == EINPROGRESS || err == EAGAIN)
                continue;

            ssize_t n = aio_return(&c->cb);
            c->pending = 0;

            if (err != 0 || n <= 0)
            {
                if (err != 0)
                    fprintf(stderr, "AIO error: %s\n",
                            strerror(err));

                close_client(c, &count);
                continue;
            }

            c->byte = toupper((unsigned char)c->byte);

            if (write(STDOUT_FILENO, &c->byte, 1) == -1)
            {
                perror("write");
                running = 0;
                break;
            }

            if (arm_read(c) == -1)
            {
                close_client(c, &count);
            }
        }

        struct timespec delay = {0, 10000000L};
        nanosleep(&delay, NULL);
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        struct Client *c = &clients[i];

        if (c->fd == -1)
            continue;

        if (c->pending)
        {
            aio_cancel(c->fd, &c->cb);

            while (aio_error(&c->cb) == EINPROGRESS)
            {
                struct timespec delay = {0, 1000000L};
                nanosleep(&delay, NULL);
            }

            aio_return(&c->cb);
        }

        close(c->fd);
    }

    close(server_fd);
    unlink(SOCKET_PATH);

    printf("\nServer stopped.\n");

    return 0;
}