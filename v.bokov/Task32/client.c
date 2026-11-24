#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(int argc, char **argv)
{
    const char *name = argc > 1 ? argv[1] : "client";

    const char *messages[] = {
        "hello from client\n",
        "this is a test message\n",
        "asynchronous io is awesome\n"
    };

    signal(SIGPIPE, SIG_IGN);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd == -1)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(fd, (struct sockaddr *)&addr,
                sizeof(addr)) == -1)
    {
        perror("connect");
        close(fd);
        return 1;
    }

    printf("[%s] Connected!\n", name);

    for (int i = 0; i < 3; i++)
    {
        char buffer[256];

        snprintf(buffer, sizeof(buffer),
                 "[%.60s] %s", name, messages[i]);

        size_t len = strlen(buffer);
        size_t sent = 0;

        while (sent < len)
        {
            ssize_t n = write(fd, buffer + sent,
                              len - sent);

            if (n == -1 && errno == EINTR)
                continue;

            if (n <= 0)
            {
                perror("write");
                close(fd);
                return 1;
            }

            sent += (size_t)n;
        }

        printf("[%s] Message %d sent.\n", name, i + 1);

        struct timespec delay = {0, 500000000L};
        nanosleep(&delay, NULL);
    }

    close(fd);

    printf("[%s] Disconnected.\n", name);

    return 0;
}