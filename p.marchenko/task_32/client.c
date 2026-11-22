#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "./socket"

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_un addr;
    const char *name = (argc > 1) ? argv[1] : "client";
    char buf[256];

    /* Чтобы клиент не падал при отключении сервера */
    signal(SIGPIPE, SIG_IGN);

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) { perror("socket"); exit(1); }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect"); exit(1);
    }
    printf("[%s] Подключен к серверу.\n", name);

    const char *messages[] = {
        "hello from client\n",
        "this is a test message\n",
        "asynchronous io is awesome\n"
    };
    int i;
    for (i = 0; i < 3; i++) {
        snprintf(buf, sizeof(buf), "[%s] %s", name, messages[i]);
        if (write(sock, buf, strlen(buf)) == -1) {
            perror("write");
            break;
        }
        sleep(5); /* Пауза 5 секунд между сообщениями */
    }

    close(sock);
    return 0;
}
