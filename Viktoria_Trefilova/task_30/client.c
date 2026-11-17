#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(void)
{
    struct sockaddr_un addr = { .sun_family = AF_UNIX, .sun_path = "socket" };
    int fd, c;
    unsigned char byte;

    signal(SIGPIPE, SIG_IGN);
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1 || connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("socket or connect");
        return 1;
    }

    while ((c = getchar()) != EOF) {
        byte = (unsigned char)c;
        if (write(fd, &byte, 1) != 1) {
            perror("write");
            break;
        }
    }

    close(fd);
    return c != EOF || ferror(stdin);
}
