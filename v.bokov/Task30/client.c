#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define TIMEOUT 10

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
        size_t len = 0;
        time_t deadline = time(NULL) + TIMEOUT;
        int timed_out = 0;

        printf("> ");
        fflush(stdout);

        while (len < sizeof(text) - 1)
        {
            time_t now = time(NULL);
            int remaining = (int)(deadline - now);

            if (remaining <= 0)
            {
                timed_out = 1;
                break;
            }

            pfd.revents = 0;

            int result = poll(&pfd, 1, remaining * 1000);

            if (result == 0)
            {
                timed_out = 1;
                break;
            }

            if (result < 0)
            {
                if (errno == EINTR)
                    continue;

                perror("poll");
                close(sock_fd);
                return 1;
            }

            if (pfd.revents & (POLLERR | POLLNVAL))
            {
                fprintf(stderr, "Input error\n");
                close(sock_fd);
                return 1;
            }

            if (pfd.revents & (POLLIN | POLLHUP))
            {
                char ch;
                ssize_t n = read(STDIN_FILENO, &ch, 1);

                if (n == 0)
                {
                    close(sock_fd);
                    printf("\nClient disconnected.\n");
                    return 0;
                }

                if (n < 0)
                {
                    if (errno == EINTR)
                        continue;

                    perror("read");
                    close(sock_fd);
                    return 1;
                }

                text[len++] = ch;

                if (ch == '\n')
                    break;
            }
        }

        if (timed_out)
        {
            printf("\n10 seconds passed! Disconnecting...\n");
            break;
        }

        text[len] = '\0';

        if (len == 1 && text[0] == '\n')
            break;

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