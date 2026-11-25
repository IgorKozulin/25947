#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define LIMIT 100

static volatile sig_atomic_t running = 1;

static void stop(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "socket"
    };
    struct pollfd fds[LIMIT + 1];
    struct sigaction action = {0};
    int server, client, i;
    unsigned char buffer[1024];
    ssize_t n;

    action.sa_handler = stop;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) == -1)
        return 1;

    server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server == -1) {
        perror("socket");
        return 1;
    }

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server);
        return 1;
    }

    if (listen(server, LIMIT) == -1) {
        perror("listen");
        close(server);
        unlink(addr.sun_path);
        return 1;
    }

    for (i = 0; i <= LIMIT; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }
    fds[0].fd = server;

    while (running) {
        if (poll(fds, LIMIT + 1, 1000) == -1) {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            client = accept(server, NULL, NULL);
            if (client >= 0) {
                for (i = 1; i <= LIMIT; i++)
                    if (fds[i].fd == -1)
                        break;
                if (i <= LIMIT)
                    fds[i].fd = client;
                else
                    close(client);
            }
        }

        for (i = 1; i <= LIMIT; i++) {
            if (fds[i].fd == -1 ||
                !(fds[i].revents & (POLLIN | POLLHUP | POLLERR)))
                continue;

            n = read(fds[i].fd, buffer, sizeof(buffer));
            if (n > 0) {
                for (ssize_t j = 0; j < n; j++)
                    putchar(toupper(buffer[j]));
                fflush(stdout);
            } else if (n == 0 || errno != EINTR) {
                close(fds[i].fd);
                fds[i].fd = -1;
            }
        }
    }

    for (i = 0; i <= LIMIT; i++)
        if (fds[i].fd != -1)
            close(fds[i].fd);

    unlink(addr.sun_path);
    return 0;
}
