#include <ctype.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(void)
{
    struct sockaddr_un addr = { .sun_family = AF_UNIX, .sun_path = "socket" };
    int server, client, result = 1;
    unsigned char byte;
    ssize_t n;

    server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server == -1 || bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("socket or bind");
        return 1;
    }

    if (listen(server, 1) == -1 || (client = accept(server, NULL, NULL)) == -1) {
        perror("listen or accept");
    } else {
        while ((n = read(client, &byte, 1)) == 1)
            putchar(toupper(byte));
        result = n < 0 || fflush(stdout) == EOF;
        close(client);
    }

    close(server);
    unlink(addr.sun_path);
    return result;
}
