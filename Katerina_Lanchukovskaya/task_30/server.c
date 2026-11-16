#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(){
    int server_fd, client_fd;
    struct sockaddr_un address;
    char buffer[256];
    ssize_t n;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd == -1){
        perror("socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1){
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

    printf("Server started. Waiting for client...\n");
    fflush(stdout);

    client_fd = accept(server_fd, NULL, NULL);

    if (client_fd == -1){
        perror("accept");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("Client connected!\n");
    fflush(stdout);

    while (1){
        n = read(client_fd, buffer, sizeof(buffer));

        if (n == 0){
            break;
        }

        if (n == -1){
            if (errno == EINTR){
                continue;
            }

            perror("read");
            break;
        }

        for (ssize_t i = 0; i < n; i++){
            buffer[i] = toupper((unsigned char)buffer[i]);
        }

        write(STDOUT_FILENO, buffer, n);
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    printf("Client disconnected. Server finished.\n");

    return 0;
}
