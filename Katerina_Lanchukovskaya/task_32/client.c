
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#define SOCKET_PATH "./socket"

int main(int argc, char *argv[]){
    int sock_fd;
    struct sockaddr_un address;
    char buffer[256];

    const char *name = argc > 1 ? argv[1] : "client";

    const char *messages[] = {
        "hello from client\n",
        "this is a test message\n",
        "asynchronous io is awesome\n"
    };

    signal(SIGPIPE, SIG_IGN);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1){
        perror("socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    if (connect(sock_fd, (struct sockaddr *)&address,
                sizeof(address)) == -1){
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("[%s] Connected to server.\n", name);

    for (int i = 0; i < 3; i++){
        snprintf(buffer, sizeof(buffer),
                 "[%s] %s", name, messages[i]);

        size_t len = strlen(buffer);
        size_t sent = 0;

        while (sent < len){
            ssize_t n = write(sock_fd, buffer + sent, len - sent);

            if (n == -1){
                if (errno == EINTR){
                    continue;
                }

                perror("write");
                close(sock_fd);
                return 1;
            }

            if (n == 0){
                close(sock_fd);
                return 1;
            }

            sent += n;
        }

        printf("[%s] Message %d sent.\n", name, i + 1);

        struct timespec delay = {0, 500000000L};
        nanosleep(&delay, NULL);
    }

    close(sock_fd);

    printf("[%s] Disconnected.\n", name);

    return 0;
}

