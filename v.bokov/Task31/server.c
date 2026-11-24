#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 10

volatile sig_atomic_t running = 1;

void stop_server(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    int server_fd;
    struct sockaddr_un addr;
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    int clients = 0;
    char buffer[256];

    signal(SIGINT, stop_server);

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

    for (int i = 0; i <= MAX_CLIENTS; i++)
    {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }

    fds[0].fd = server_fd;

    printf("Server started. Waiting for clients...\n");
    printf("Press Ctrl+C to stop.\n");
    fflush(stdout);

    while (running)
    {
        int ready = poll(fds, nfds, 1000);

        if (ready == -1)
        {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        if (ready == 0)
            continue;

        if (fds[0].revents & POLLIN)
        {
            int client_fd = accept(server_fd, NULL, NULL);

            if (client_fd == -1)
            {
                perror("accept");
            }
            else
            {
                int slot = -1;

                for (int i = 1; i <= MAX_CLIENTS; i++)
                {
                    if (fds[i].fd == -1)
                    {
                        slot = i;
                        break;
                    }
                }

                if (slot == -1)
                {
                    printf("Too many clients!\n");
                    close(client_fd);
                }
                else
                {
                    fds[slot].fd = client_fd;
                    fds[slot].events = POLLIN;
                    fds[slot].revents = 0;

                    if (slot >= nfds)
                        nfds = slot + 1;

                    clients++;

                    printf("Client connected (fd=%d). Total: %d\n",
                           client_fd, clients);
                }
            }
        }

        for (int i = 1; i < nfds; i++)
        {
            if (fds[i].fd == -1)
                continue;

            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL))
            {
                ssize_t n = read(fds[i].fd, buffer,
                                 sizeof(buffer));

                if (n > 0)
                {
                    for (ssize_t j = 0; j < n; j++)
                    {
                        buffer[j] = toupper(
                            (unsigned char)buffer[j]);
                    }

                    printf("[Client %d]: ", fds[i].fd);
                    fwrite(buffer, 1, n, stdout);
                    fflush(stdout);
                }
                else if (n == 0 ||
                         (n == -1 && errno != EINTR))
                {
                    printf("Client %d disconnected.\n",
                           fds[i].fd);

                    close(fds[i].fd);
                    fds[i].fd = -1;
                    fds[i].revents = 0;
                    clients--;

                    printf("Clients remaining: %d\n", clients);
                }
            }
        }

        while (nfds > 1 && fds[nfds - 1].fd == -1)
            nfds--;
    }

    for (int i = 1; i < nfds; i++)
    {
        if (fds[i].fd != -1)
            close(fds[i].fd);
    }

    close(server_fd);
    unlink(SOCKET_PATH);

    printf("\nServer stopped.\n");

    return 0;
}