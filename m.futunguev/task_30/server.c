#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int main(void) {
    unlink(SOCKET_PATH);                                // удаляем старый файл сокета

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);    // создаём сокет

    struct sockaddr_un addr;                            // адрес сокета
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));   // привязываем к файлу
    listen(server_fd, 5);                                      // ждём клиентов

    int client_fd = accept(server_fd, NULL, NULL);      // ждём подключения клиента

    char buf[1024];
    ssize_t n;
    while ((n = read(client_fd, buf, sizeof(buf))) > 0) {   // читаем от клиента, пока не EOF
        for (ssize_t i = 0; i < n; i++)
            buf[i] = toupper((unsigned char)buf[i]);        // в верхний регистр
        write(client_fd, buf, n);                           // отправляем результат обратно клиенту
    }

    close(client_fd);                                   // клиент отключился - закрываем всё
    close(server_fd);
    unlink(SOCKET_PATH);                                // удаляем файл сокета
    return 0;
}

/*
Как работает сервер:
unlink() - удаляет старый файл сокета, если он остался.
socket() - создаёт сокет.
bind() - привязывает сокет к файлу ./socket (файл появляется).
listen() - включает режим ожидания клиентов.
accept() - ждёт клиента и возвращает дескриптор для общения с ним.
В цикле read() получает текст от клиента, toupper() делает буквы заглавными,
write() отправляет результат ОБРАТНО клиенту (в тот же client_fd).
Когда read() вернул 0 (клиент закрыл соединение), закрывает сокеты и удаляет файл через unlink().
*/