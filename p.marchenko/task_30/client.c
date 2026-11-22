#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/my_socket_30"

int main(void) {
    int sock;
    struct sockaddr_un addr;
    char buffer[1024];

    /* 1. Создаем сокет */
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) { perror("socket"); exit(1); }

    /* 2. Указываем адрес сервера */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    /* 3. Подключаемся */
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect"); exit(1);
    }
    printf("[CLIENT] Подключено к серверу. Вводите текст (Ctrl+D для выхода):\n");

    /* 4. Читаем с клавиатуры и отправляем серверу */
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (write(sock, buffer, strlen(buffer)) == -1) {
            perror("write");
            break;
        }
    }

    close(sock);
    return 0;
}
