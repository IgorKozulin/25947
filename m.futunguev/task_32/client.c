/*
Отличие от задачи 31:
Клиент почти не изменился - асинхронное чтение нужно только серверу,
клиенту всё равно, как сервер читает данные. Добавлено только
signal(SIGPIPE, SIG_IGN), чтобы клиент не падал, если сервер закрылся,
и проверка результата write.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h> // изменено: signal, SIGPIPE
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN); // изменено: если сервер закрылся - не падаем при записи

    const char *name = (argc > 1) ? argv[1] : "client"; // имя клиента для наглядности

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0); // создаём сокет

    struct sockaddr_un addr; // адрес сервера
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) { // подключаемся
        perror("connect"); // сервер не запущен
        return 1;
    }

    printf("Вводи текст (0 - выход):\n");

    char line[1024]; // строка с клавиатуры
    char msg[1100]; // строка с именем клиента
    while (fgets(line, sizeof(line), stdin) != NULL) { // читаем строку с клавиатуры
        if (strcmp(line, "0\n") == 0) // ввели 0 - выходим
            break;

        int len = snprintf(msg, sizeof(msg), "[%s] %s", name, line); // "[Alice] текст"
        if (write(sock_fd, msg, len) == -1) { // изменено: отправляем и проверяем результат
            perror("write"); // сервер закрылся
            break;
        }

        // ответ от сервера не ждём - сервер печатает текст сам
    }

    close(sock_fd); // закрываем, сервер получит EOF и отключит этого клиента
    return 0;
}
