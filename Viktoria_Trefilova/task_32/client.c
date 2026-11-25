#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

int main(void)
{
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "socket"
    };
    int fd, c, result = 0;
    unsigned char byte;
    ssize_t n;
    time_t start = time(NULL);

    signal(SIGPIPE, SIG_IGN);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }

    while ((c = getchar()) != EOF) {
        byte = (unsigned char)c;
        do {
            n = write(fd, &byte, 1);
        } while (n == -1 && errno == EINTR);

        if (n != 1) {
            perror("write");
            result = 1;
            break;
        }
    }

    close(fd);
    printf("\nClient finished. Lifetime: %.0f seconds\n",
           difftime(time(NULL), start));

    return result || ferror(stdin);
}
