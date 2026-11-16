
#include <stdio.h>
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

volatile sig_atomic_t finished = 0;

void sigint_handler(int sig){
    finished = 1;
}

int main(){
    int server_fd, client_fd;
    int clients = 0;
    struct sockaddr_un address;
    struct pollfd fds[MAX_CLIENTS + 1];
    nfds_t nfds = 1;
    char buffer[256];
    ssize_t n;

    signal(SIGINT, sigint_handler);

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

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    for (int i = 1; i <= MAX_CLIENTS; i++){
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }

    printf("[SERVER] Started. Waiting for clients...\n");
    printf("[SERVER] Press Ctrl+C to stop.\n");
    fflush(stdout);

    while (!finished){
        int ready = poll(fds, nfds, 1000);

        if (ready == -1){
            if (errno == EINTR){
                continue;
            }
            perror("poll");
            break;
        }

        if (ready == 0){
            continue;
        }

        if (fds[0].revents & POLLIN){
            client_fd = accept(server_fd, NULL, NULL);

            if (client_fd != -1){
                int slot = -1;

                for (int i = 1; i <= MAX_CLIENTS; i++){
                    if (fds[i].fd == -1){
                        slot = i;
                        break;
                    }
                }

                if (slot == -1){
                    close(client_fd);
                }
                else {
                    fds[slot].fd = client_fd;
                    fds[slot].events = POLLIN;
                    fds[slot].revents = 0;

                    if ((nfds_t)(slot + 1) > nfds){
                        nfds = slot + 1;
                    }

                    clients++;
                    printf("[SERVER] Client connected (fd=%d). Total: %d\n",
                           client_fd, clients);
                    fflush(stdout);
                }
            }
        }

        for (nfds_t i = 1; i < nfds; i++){
            if (fds[i].fd == -1){
                continue;
            }

            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)){
                n = read(fds[i].fd, buffer, sizeof(buffer));

                if (n > 0){
                    for (ssize_t j = 0; j < n; j++){
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }

                    printf("[Client %d]: ", fds[i].fd);
                    fflush(stdout);
                    write(STDOUT_FILENO, buffer, n);
                }
                else if (n == 0 || (n == -1 && errno != EINTR)){
                    printf("[SERVER] Client %d disconnected.\n", fds[i].fd);

                    close(fds[i].fd);
                    fds[i].fd = -1;
                    clients--;

                    printf("[SERVER] Remaining clients: %d\n", clients);
                    fflush(stdout);
                }
            }
        }

        while (nfds > 1 && fds[nfds - 1].fd == -1){
            nfds--;
        }
    }

    for (nfds_t i = 1; i < nfds; i++){
        if (fds[i].fd != -1){
            close(fds[i].fd);
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);

    printf("[SERVER] Finished.\n");

    return 0;
}

