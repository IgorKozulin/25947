#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>

#define SOCKET_PATH "./socket"

/* Надёжная запись (write может записать меньше, чем просили) */
static int write_all(int fd, const char *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t n = write(fd, buf + total, count - total);
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += n;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    const char *name = (argc > 1) ? argv[1] : "CLIENT";
    int sock_fd;
    struct sockaddr_un addr;

    signal(SIGPIPE, SIG_IGN); // не падать, если сервер закрыл соединение

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect"); exit(EXIT_FAILURE);
    }

    printf("[%s] Подключен к серверу. Отправка сообщений...\n", name);

    char msg[256];
    for (int i = 0; i < 3; i++) {
        snprintf(msg, sizeof(msg), "[%s] Hello from client! (msg %d)\n", name, i + 1);
        printf("[%s] Отправляю: %s", name, msg);

        if (write_all(sock_fd, msg, strlen(msg)) == -1) {
            perror("write"); break;
        }
        usleep(500000); // 0.5 сек — чтобы в сервере было видно чередование
    }

    printf("[%s] Все сообщения отправлены. Закрытие.\n", name);
    close(sock_fd);
    return EXIT_SUCCESS;
}
