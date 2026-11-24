#define _POSIX_C_SOURCE 200809L
#include "socket_io.h"

int main(int argc, char *argv[])
{

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [socket_path]\n", argv[0]);
        return 1;
    }
    const char *path = argc == 2 ? argv[1] : "./socket";
    if (setup_signals(1) == -1) {
        perror("signal setup");
        return 1;
    }
    int server = open_server(path);
    if (server == -1) {
        perror("server socket");
        return 1;
    }
    fprintf(stderr, "Server start up. waiting connections on %s...\n", path);

    int client;
    do {
        client = accept(server, NULL, NULL);
        fprintf(stderr, "New connection!\n");
    } while (client == -1 && errno == EINTR);
    if (client == -1) {
        perror("accept");
        close(server);
        return 1;
    }

    unsigned char buffer[1024];
    int status = 0;
    while (1) {
        ssize_t length = read(client, buffer, sizeof(buffer));
        if (length == -1 && errno == EINTR) continue;
        if (length == 0) break;
        if (length == -1) {
            perror("read");
            status = 1;
            break;
        }
        if (print_upper(buffer, (size_t)length) == -1) {
            perror("stdout");
            status = 1;
            break;
        }
    }
    close(client);
    close(server);
    return status;
}
