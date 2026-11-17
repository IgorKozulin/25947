#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PATH "/tmp/task31.sock"

int main(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, PATH);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }

    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (write(fd, buffer, strlen(buffer)) == -1) {
            perror("write");
            break;
        }
    }

    close(fd);
    return 0;
}

