#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int write_all(int fd, const char *buffer, size_t length){
    size_t sent = 0;

    while (sent < length){
        ssize_t n = write(fd, buffer + sent, length - sent);

        if (n == -1){
            if (errno == EINTR){
                continue;
            }

            return -1;
        }

        if (n == 0){
            return -1;
        }

        sent += n;
    }

    return 0;
}

int main(){
    int sock_fd;
    struct sockaddr_un address;
    char buffer[256];

    signal(SIGPIPE, SIG_IGN);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1){
        perror("socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    if (connect(sock_fd, (struct sockaddr *)&address, sizeof(address)) == -1){
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("Connected to server.\n");
    printf("Enter text (empty line or Ctrl+D to exit):\n");

    while (fgets(buffer, sizeof(buffer), stdin) != NULL){
        if (strcmp(buffer, "\n") == 0){
            break;
        }

        if (write_all(sock_fd, buffer, strlen(buffer)) == -1){
            perror("write");
            close(sock_fd);
            return 1;
        }
    }

    close(sock_fd);

    printf("Client finished.\n");

    return 0;
}
