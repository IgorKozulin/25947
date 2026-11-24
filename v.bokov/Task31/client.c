#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s ClientName\n", argv[0]);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);

    int sock_fd;
    struct sockaddr_un addr;
    char message[256];

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

    printf("[%s] Connected to server!\n", argv[1]);

    const char *texts[] = {
        "Hello from client!",
        "This is a test message.",
        "Unix domain sockets are working."
    };

    for (int i = 0; i < 3; i++)
    {
        snprintf(message, sizeof(message),
                 "[%.60s] %s\n", argv[1], texts[i]);

        size_t len = strlen(message);
        size_t sent = 0;

        while (sent < len)
        {
            ssize_t n = write(sock_fd,
                              message + sent,
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

        printf("[%s] Sent: %s\n", argv[1], texts[i]);
        sleep(1);
    }

    close(sock_fd);

    printf("[%s] Disconnected.\n", argv[1]);

    return 0;
}