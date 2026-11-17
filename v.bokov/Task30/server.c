#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define TIMEOUT 10000

int main(void)
{
    int sock_fd;
    struct sockaddr_un addr;
    struct pollfd pfd;
    char text[256];

    signal(SIGPIPE, SIG_IGN);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1)
    {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock_fd, (struct sockaddr *)&addr,
                sizeof(addr)) == -1)
    {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("Connected to server!\n");
    printf("You have 10 seconds to enter each line.\n");

    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    while (1)
    {
        printf("> ");
        fflush(stdout);

        int result = poll(&pfd, 1, TIMEOUT);

        if (result == 0)
        {
            printf("\n10 seconds passed! Disconnecting...\n");
            break;
        }

        if (result < 0)
        {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        if (fgets(text, sizeof(text), stdin) == NULL)
            break;

        if (strcmp(text, "\n") == 0)
            break;

        size_t len = strlen(text);
        size_t sent = 0;

        while (sent < len)
        {
            ssize_t n = write(sock_fd,
                              text + sent,
                              len - sent);

            if (n == -1 && errno == EINTR)
                continue;

            if (n <= 0)
            {
                perror("write");
                close(sock_fd);
                return 1;
            }

            sent += (size_t)n;
        }
    }

    close(sock_fd);

    printf("Client disconnected.\n");

    return 0;
}