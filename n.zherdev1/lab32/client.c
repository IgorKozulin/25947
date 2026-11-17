#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/task32.sock"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s \"text\"\n", argv[0]);
        return 1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }

    write(fd, argv[1], strlen(argv[1]));
    write(fd, "\n", 1);

    close(fd);

    return 0;
}
