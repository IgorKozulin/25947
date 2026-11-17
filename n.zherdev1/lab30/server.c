#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PATH "/tmp/task30.sock"

int main(void)
{
    int server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, PATH);

    unlink(PATH);

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1 ||
        listen(server, 1) == -1) {
        perror("bind/listen");
        return 1;
    }

    int client = accept(server, NULL, NULL);
    if (client == -1) {
        perror("accept");
        return 1;
    }

    char buf[256];
    ssize_t n;

    while ((n = read(client, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++)
            buf[i] = toupper((unsigned char)buf[i]);

        write(STDOUT_FILENO, buf, n);
    }

    close(client);
    close(server);
    unlink(PATH);

    return 0;
}

