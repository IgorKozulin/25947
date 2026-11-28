#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 16

static volatile sig_atomic_t stop = 0;

static void on_interrupt(int signo)
{
    (void)signo;
    stop = 1;
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

static void add_client(struct pollfd fds[], nfds_t *nfds)
{
    int fd = accept(fds[0].fd, NULL, NULL);
    if (fd == -1) {
        if (errno != EINTR)
            perror("accept");
        return;
    }

    for (nfds_t i = 1; i <= MAX_CLIENTS; i++) {
        if (fds[i].fd == -1) {
            fds[i] = (struct pollfd){ .fd = fd, .events = POLLIN };
            if (i >= *nfds)
                *nfds = i + 1;

            fprintf(stderr, "Client connected (fd=%d)\n", fd);
            return;
        }
    }

    fputs("Too many clients\n", stderr);
    close(fd);
}

static void remove_client(struct pollfd *client)
{
    fprintf(stderr, "Client disconnected (fd=%d)\n", client->fd);
    close(client->fd);
    client->fd = -1;
}

int main(void)
{
    struct sigaction action = {0};
    action.sa_handler = on_interrupt;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) == -1) {
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

    struct pollfd fds[MAX_CLIENTS + 1];
    fds[0] = (struct pollfd){ .fd = server_fd, .events = POLLIN };

    for (nfds_t i = 1; i <= MAX_CLIENTS; i++)
        fds[i] = (struct pollfd){ .fd = -1 };

    nfds_t nfds = 1;
    int status = 0;

    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        status = 1;
        goto done;
    }

    fputs("Listening on " SOCKET_PATH ". Press Ctrl+C to stop.\n", stderr);

    while (!stop) {
        int ready = poll(fds, nfds, 1000);
        if (ready == -1) {
            if (errno == EINTR)
                continue;

            perror("poll");
            status = 1;
            break;
        }

        if (ready == 0 || stop)
            continue;

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fputs("Listening socket error\n", stderr);
            status = 1;
            break;
        }

        if (fds[0].revents & POLLIN)
            add_client(fds, &nfds);

        for (nfds_t i = 1; i < nfds; i++) {
            if (fds[i].fd == -1)
                continue;

            short events = fds[i].revents;
            if (events & (POLLIN | POLLHUP)) {
                unsigned char buffer[1024];
                ssize_t n = read(fds[i].fd, buffer, sizeof(buffer));

                if (n == -1 && errno == EINTR)
                    continue;

                if (n <= 0) {
                    if (n == -1)
                        perror("read");
                    remove_client(&fds[i]);
                    continue;
                }

                for (ssize_t j = 0; j < n; j++)
                    buffer[j] = (unsigned char)toupper(buffer[j]);

                if (fwrite(buffer, 1, (size_t)n, stdout) != (size_t)n ||
                    fflush(stdout) == EOF) {
                    perror("stdout");
                    status = 1;
                    goto done;
                }
            } else if (events & (POLLERR | POLLNVAL)) {
                remove_client(&fds[i]);
            }
        }

        while (nfds > 1 && fds[nfds - 1].fd == -1)
            nfds--;
    }

done:
    for (nfds_t i = 0; i < nfds; i++) {
        if (fds[i].fd != -1)
            close(fds[i].fd);
    }

    if (unlink(SOCKET_PATH) == -1) {
        perror("unlink");
        status = 1;
    }

    return status;
}
