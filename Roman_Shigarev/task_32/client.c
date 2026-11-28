#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define SOCKET_PATH "./socket"

static int write_all(int fd, const char *text, size_t length)
{
    while (length > 0) {
        ssize_t n = write(fd, text, length);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        text += n;
        length -= (size_t)n;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc > 2 || (argc == 2 && argv[1][0] == '\0')) {
        fprintf(stderr, "Usage: %s [name]\n", argv[0]);
        return 1;
    }
    const char *name = argc == 2 ? argv[1] : "client";

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

    const char *messages[] = {
        "hello from client\n",
        "this is a test message\n",
        "asynchronous io is awesome\n"
    };
    const size_t count = sizeof(messages) / sizeof(messages[0]);
    const struct timespec delay = {0, 500000000L};

    for (size_t i = 0; i < count; i++) {
        char line[256];
        int length = snprintf(line, sizeof(line), "[%s] %s", name, messages[i]);

        if (length < 0 || (size_t)length >= sizeof(line)) {
            fputs("Cannot format message: client name is too long\n", stderr);
            close(fd);
            return 1;
        }

        if (write_all(fd, line, (size_t)length) == -1) {
            perror("write");
            close(fd);
            return 1;
        }

        printf("Sent: %s", line);
        fflush(stdout);

        if (i + 1 < count)
            nanosleep(&delay, NULL);
    }

    close(fd);
    return 0;
}
