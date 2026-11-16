#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(void) {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);      // создаём сокет

    struct sockaddr_un addr;                            // адрес сервера
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {   // подключаемся
        perror("connect");                              // сервер не запущен
        return 1;
    }

    printf("Вводи текст (0 - выход):\n");

    char line[1024];                                    // строка с клавиатуры
    char answer[1024];                                  // ответ от сервера
    while (fgets(line, sizeof(line), stdin) != NULL) {  // читаем строку с клавиатуры
        if (strcmp(line, "0\n") == 0)                   // ввели 0 - выходим
            break;

        write(sock_fd, line, strlen(line));             // отправляем строку серверу

        ssize_t n = read(sock_fd, answer, sizeof(answer));   // ждём ответ сервера
        if (n <= 0)                                     // сервер закрыл соединение или ошибка
            break;
        write(1, answer, n);                            // выводим ответ на экран
    }

    close(sock_fd);                                     // закрываем, сервер получит EOF и завершится
    return 0;
}


/*
Как работает клиент:
socket() - создаёт сокет.
connect() - подключается к файлу ./socket.
Читаем строки с клавиатуры (fgets) и отправляем серверу через write().
Затем read() ждёт ответ сервера (строку в верхнем регистре) и write(1, ...) выводит его на экран.
При вводе 0 выходит из цикла и закрывает сокет через close(). Это и есть сигнал серверу, что пора завершаться.
*/