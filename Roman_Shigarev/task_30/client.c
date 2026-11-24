#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "./socket"

static int write_all(int fd, const char *text)
{
    size_t left = strlen(text);
    while (left > 0) {
        ssize_t n = write(fd, text, left);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        text += n;
        left -= (size_t)n;
    }
    return 0;
}

int main(void)
{
    const char *messages[] = {
        "Hello World!\n",
        "This is a Test Message!!!\n",
        "Unix domain sockets are awesome.\n"
    };

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un address = {0};
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }

    for (size_t i = 0; i < sizeof(messages) / sizeof(messages[0]); i++) {
        if (write_all(fd, messages[i]) == -1) {
            perror("write");
            close(fd);
            return 1;
        }
        printf("[CLIENT] Sent: %s", messages[i]);
    }

    close(fd);
    return 0;
}
