#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PATH "/tmp/task31.sock"
#define MAX_CLIENTS 10

volatile sig_atomic_t running = 1;

void stop(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    int server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, PATH);

    unlink(PATH);

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1 ||
        listen(server, 5) == -1) {
        perror("server");
        close(server);
        unlink(PATH);
        return 1;
    }

    struct pollfd fds[MAX_CLIENTS + 1];

    for (int i = 0; i <= MAX_CLIENTS; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }

    fds[0].fd = server;
    signal(SIGINT, stop);

    while (running) {
        if (poll(fds, MAX_CLIENTS + 1, 1000) == -1) {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int client = accept(server, NULL, NULL);

            if (client != -1) {
                int i;
                for (i = 1; i <= MAX_CLIENTS; i++) {
                    if (fds[i].fd == -1) {
                        fds[i].fd = client;
                        fds[i].revents = 0;
                        break;
                    }
                }

                if (i > MAX_CLIENTS)
                    close(client);
            }
        }

        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (fds[i].fd == -1 || !fds[i].revents)
                continue;

            char buffer[1024];
            ssize_t n = read(fds[i].fd, buffer, sizeof(buffer));

            if (n <= 0) {
                close(fds[i].fd);
                fds[i].fd = -1;
                continue;
            }

            for (ssize_t j = 0; j < n; j++)
                buffer[j] = toupper((unsigned char)buffer[j]);

            write(STDOUT_FILENO, buffer, n);
        }
    }

    for (int i = 0; i <= MAX_CLIENTS; i++)
        if (fds[i].fd != -1)
            close(fds[i].fd);

    unlink(PATH);
    return 0;
}

