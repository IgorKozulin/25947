#include <aio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 16

struct client {
    int fd;
    int pending;
    unsigned char byte;
    struct aiocb request;
};

static volatile sig_atomic_t stop = 0;

static void on_stop(int signo)
{
    (void)signo;
    stop = 1;
}

static void short_pause(void)
{
    const struct timespec delay = {0, 10000000L};
    nanosleep(&delay, NULL);
}

static int set_nonblocking(int fd, int enabled)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        return -1;

    return fcntl(fd, F_SETFL,
                 enabled ? flags | O_NONBLOCK : flags & ~O_NONBLOCK);
}

static int remove_old_socket(void)
{
    struct stat info;
    if (lstat(SOCKET_PATH, &info) == -1) {
        if (errno == ENOENT)
            return 0;
        perror("lstat");
        return -1;
    }

    if (!S_ISSOCK(info.st_mode)) {
        fputs(SOCKET_PATH " is not a socket\n", stderr);
        return -1;
    }

    if (unlink(SOCKET_PATH) == -1) {
        perror("unlink");
        return -1;
    }
    return 0;
}

static int start_read(struct client *c)
{
    memset(&c->request, 0, sizeof(c->request));
    c->request.aio_fildes = c->fd;
    c->request.aio_buf = &c->byte;
    c->request.aio_nbytes = 1;
    c->request.aio_sigevent.sigev_notify = SIGEV_NONE;

    if (aio_read(&c->request) == 0) {
        c->pending = 1;
    } else if (errno != EAGAIN && errno != EINTR && errno != EINPROGRESS) {
        perror("aio_read");
        return -1;
    }

    return 0;
}

static void remove_client(struct client *c)
{
    fprintf(stderr, "Client disconnected (fd=%d)\n", c->fd);
    close(c->fd);
    c->fd = -1;
}

static int accept_clients(int server_fd, struct client clients[])
{
    while (!stop) {
        int fd = accept(server_fd, NULL, NULL);
        if (fd == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;
            perror("accept");
            return -1;
        }

        int i = 0;
        while (i < MAX_CLIENTS && clients[i].fd != -1)
            i++;

        if (i == MAX_CLIENTS) {
            fputs("Too many clients\n", stderr);
            close(fd);
        } else if (set_nonblocking(fd, 0) == -1) {
            perror("fcntl (client)");
            close(fd);
        } else {
            clients[i].fd = fd;
            clients[i].pending = 0;
            fprintf(stderr, "Client connected (fd=%d)\n", fd);
        }
    }
    return 0;
}

static int handle_client(struct client *c)
{
    if (c->pending) {
        int error = aio_error(&c->request);

        if (error == EINPROGRESS || error == EAGAIN)
            return 0;

        c->pending = 0;

        if (error == -1) {
            perror("aio_error");
            remove_client(c);
            return 0;
        }

        ssize_t n = aio_return(&c->request);
        if (error != 0 || n <= 0) {
            if (error != 0 && error != ECANCELED)
                fprintf(stderr, "AIO read: %s\n", strerror(error));
            remove_client(c);
            return 0;
        }

        unsigned char upper = (unsigned char)toupper(c->byte);
        ssize_t written;
        do {
            written = write(STDOUT_FILENO, &upper, 1);
        } while (written == -1 && errno == EINTR && !stop);

        if (written != 1) {
            if (stop)
                return 0;
            perror("write");
            return -1;
        }
    }

    if (!stop && start_read(c) == -1)
        remove_client(c);

    return 0;
}

static void finish_client(struct client *c)
{
    if (c->pending) {
        aio_cancel(c->fd, &c->request);
        shutdown(c->fd, SHUT_RDWR);

        int error;
        while ((error = aio_error(&c->request)) == EINPROGRESS)
            short_pause();

        if (error != -1)
            aio_return(&c->request);
    }
    close(c->fd);
}

int main(void)
{
    struct sigaction action = {0};
    action.sa_handler = on_stop;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) == -1 ||
        sigaction(SIGTERM, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    if (remove_old_socket() == -1)
        return 1;

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un address = {0};
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    struct client clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    int status = 0;
    if (listen(server_fd, MAX_CLIENTS) == -1 ||
        set_nonblocking(server_fd, 1) == -1) {
        perror("listen/fcntl");
        status = 1;
        goto done;
    }

    fputs("AIO server listening on " SOCKET_PATH ". Press Ctrl+C to stop.\n",
          stderr);

    while (!stop) {
        if (accept_clients(server_fd, clients) == -1) {
            status = 1;
            break;
        }

        for (int i = 0; i < MAX_CLIENTS && !stop; i++) {
            if (clients[i].fd != -1 && handle_client(&clients[i]) == -1) {
                status = 1;
                goto done;
            }
        }
        if (!stop)
            short_pause();
    }

done:
    close(server_fd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1)
            finish_client(&clients[i]);
    }
    if (unlink(SOCKET_PATH) == -1) {
        perror("unlink");
        status = 1;
    }
    return status;
}
