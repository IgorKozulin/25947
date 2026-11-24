#define _DARWIN_C_SOURCE
#define _DEFAULT_SOURCE
#define __EXTENSIONS__
#define _POSIX_C_SOURCE 200809L
#include <aio.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#ifdef __sun
#include <sys/filio.h>
#endif
#include "socket_io.h"

#define MAX_CLIENTS 64

static volatile sig_atomic_t stopped = 0;

static void wake_server(int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM) stopped = 1;
}

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


struct Client {
    int fd;
    int active;
    unsigned char byte;
    struct aiocb request;
    struct ClientLine line;
};

int arm_read(struct Client *client)
{
    memset(&client->request, 0, sizeof(client->request));
    client->request.aio_fildes = client->fd;
    client->request.aio_buf = &client->byte;
    /* A one-byte request also completes promptly on Solaris sockets. */
    client->request.aio_nbytes = 1;
    client->request.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    client->request.aio_sigevent.sigev_signo = SIGUSR1;
    if (aio_read(&client->request) == -1) return -1;
    client->active = 1;
    return 0;
}

int disconnect_client(struct Client *client, int *count)
{
    int result = print_line(client->fd, &client->line);
    fprintf(stderr, "[SERVER] Client %d disconnected.\n", client->fd);
    close(client->fd);
    free(client->line.text);
    memset(client, 0, sizeof(*client));
    client->fd = -1;
    (*count)--;
    fprintf(stderr, "[SERVER] Clients remaining: %d\n", *count);
    return result;
}

int main(int argc, char *argv[])
{
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [socket_path]\n", argv[0]);
        return 1;
    }
    const char *path = argc == 2 ? argv[1] : "./socket";
    sigset_t signals;
    sigset_t wait_mask;
    sigemptyset(&signals);
    sigaddset(&signals, SIGIO);
    sigaddset(&signals, SIGUSR1);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &signals, &wait_mask) == -1) {
        perror("sigprocmask");
        return 1;
    }
    if (setup_signals(1) == -1) {
        perror("signal setup");
        return 1;
    }
    struct sigaction action = {0};
    action.sa_handler = wake_server;
    sigemptyset(&action.sa_mask);
    int numbers[] = {SIGIO, SIGUSR1, SIGINT, SIGTERM};
    for (size_t i = 0; i < sizeof(numbers) / sizeof(numbers[0]); i++) {
        sigdelset(&wait_mask, numbers[i]);
        if (sigaction(numbers[i], &action, NULL) == -1) {
            perror("sigaction");
            return 1;
        }
    }
    int server = open_server(path);
    if (server == -1) {
        perror("server socket");
        return 1;
    }
    int flags = fcntl(server, F_GETFL);
    int enabled = 1;
    if (flags == -1 || fcntl(server, F_SETFL, flags | O_NONBLOCK) == -1
            || fcntl(server, F_SETOWN, getpid()) == -1
            || ioctl(server, FIOASYNC, &enabled) == -1) {
        perror("async listener");
        close(server);
        return 1;
    }
    fprintf(stderr, "[SERVER] AIO server listening on %s. Ctrl+C to stop.\n", path);
    struct Client clients[MAX_CLIENTS] = {0};
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = -1;
    int client_count = 0;
    int status = 0;
    while (!stopped) {
        int fd;
        while ((fd = accept(server, NULL, NULL)) != -1) {
            int slot = 0;
            while (slot < MAX_CLIENTS && clients[slot].fd != -1) slot++;
            if (slot == MAX_CLIENTS) {
                fprintf(stderr, "[SERVER] Too many clients.\n");
                close(fd);
                continue;
            }
            flags = fcntl(fd, F_GETFL);
            enabled = 0;
            if (flags == -1 || fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1
                    || ioctl(fd, FIOASYNC, &enabled) == -1) {
                perror("blocking client socket");
                close(fd);
                status = 1;
                goto cleanup;
            }
            clients[slot].fd = fd;
            client_count++;
            fprintf(stderr, "[SERVER] New client connected (fd=%d). Total: %d\n",
                    fd, client_count);
            if (arm_read(&clients[slot]) == -1) {
                perror("aio_read");
                status = 1;
                goto cleanup;
            }
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            perror("accept");
            status = 1;
            break;
        }
        for (int i = 0; i < MAX_CLIENTS; i++) {
            struct Client *client = &clients[i];
            if (!client->active) continue;
            int error = aio_error(&client->request);
            if (error == EINPROGRESS) continue;
            ssize_t length = aio_return(&client->request);
            client->active = 0;
            if (error != 0 || length <= 0) {
                if (error != 0) fprintf(stderr, "aio_read: %s\n", strerror(error));
                if (disconnect_client(client, &client_count) == -1) {
                    perror("stdout");
                    status = 1;
                    goto cleanup;
                }
                continue;
            }
            if (receive_lines(client->fd, &client->line, &client->byte, 1) == -1) {
                perror("client output");
                status = 1;
                goto cleanup;
            }
            if (arm_read(client) == -1) {
                perror("aio_read");
                status = 1;
                goto cleanup;
            }
        }
        /* Pending completion/connection signals are delivered atomically here. */
        sigsuspend(&wait_mask);
    }
cleanup:
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            shutdown(clients[i].fd, SHUT_RDWR);
            if (clients[i].active) aio_cancel(clients[i].fd, &clients[i].request);
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) continue;
        if (clients[i].active) {
            const struct aiocb *requests[] = {&clients[i].request};
            while (aio_error(&clients[i].request) == EINPROGRESS) {
                aio_suspend(requests, 1, NULL);
            }
            aio_return(&clients[i].request);
        }
        close(clients[i].fd);
        free(clients[i].line.text);
    }
    close(server);
    return status;
}
