#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>

#define SOCK_PATH "/tmp/my_socket_30"

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[1024];
    ssize_t n;

    /* 1. Создаем сокет */
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); exit(1); }

    /* 2. Привязываем к файлу */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCK_PATH); /* Удаляем старый файл сокета, если есть */

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind"); exit(1);
    }

    /* 3. Слушаем */
    if (listen(server_fd, 5) == -1) {
        perror("listen"); exit(1);
    }
    printf("Сервер запущен. Ожидаю клиента...\n");

    /* 4. Принимаем соединение */
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) { perror("accept"); exit(1); }
    printf("Клиент подключился!\n");

    /* 5. Читаем данные, переводим в верхний регистр и выводим */
    while ((n = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        for (int i = 0; i < n; i++) {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }
        printf("%s", buffer);
        fflush(stdout);
    }

    printf("\nКлиент отключился. Сервер завершает работу.\n");
    close(client_fd);
    close(server_fd);
    unlink(SOCK_PATH); /* Убираем за собой */
    return 0;
}
