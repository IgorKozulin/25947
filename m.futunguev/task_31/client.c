/*
Как работает клиент:
socket() - создаёт сокет.
connect() - подключается к файлу ./socket.
Читаем строки с клавиатуры (fgets), добавляем перед ними имя клиента
и отправляем серверу через write().
Ответа не ждём: сервер сам переводит текст в верхний регистр и печатает
его у себя на экране.
При вводе 0 (или Ctrl+D) выходит из цикла и закрывает сокет через close().
Сервер получает read() == 0 и отключает этого клиента, а сам продолжает
работать с остальными.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(int argc, char *argv[]) { // изменено: принимаем аргументы
    const char *name = (argc > 1) ? argv[1] : "client"; // изменено: имя клиента для наглядности

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
    char msg[1100]; // изменено: строка с именем клиента
    while (fgets(line, sizeof(line), stdin) != NULL) { // читаем строку с клавиатуры
        if (strcmp(line, "0\n") == 0) // ввели 0 - выходим
            break;

        int len = snprintf(msg, sizeof(msg), "[%s] %s", name, line); // изменено: "[Alice] текст"
        write(sock_fd, msg, len); // отправляем строку серверу

        // ответ от сервера больше не ждём - сервер печатает текст сам
    }

    close(sock_fd); // закрываем, сервер получит EOF и отключит этого клиента
    return 0;
}