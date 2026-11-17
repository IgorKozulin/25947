/*
Как работает сервер:
unlink() - удаляет старый файл сокета, если он остался.
socket() - создаёт сокет.
bind() - привязывает сокет к файлу ./socket (файл появляется).
listen() - включает режим ожидания клиентов.
poll() - ждёт, пока на любом сокете из массива что-то произойдёт.
Если событие на слушающем сокете - accept() принимает нового клиента
и он записывается в свободное место массива.
Если событие на сокете клиента - read() получает текст, toupper() делает
буквы заглавными, write(1, ...) печатает результат на экран сервера.
Когда read() вернул 0 (клиент закрыл соединение), его сокет закрывается,
а место в массиве освобождается для следующего клиента.
Так сервер обслуживает нескольких клиентов одновременно.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h> // poll, struct pollfd

#define SOCKET_PATH "./socket"
#define N 11 // изменено: 1 слушающий сокет + до 10 клиентов

int main(void) {
    setbuf(stdout, NULL); // отключаем буфер вывода
    unlink(SOCKET_PATH); // удаляем старый файл сокета

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0); // создаём сокет

    struct sockaddr_un addr; // адрес сокета
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)); // привязываем к файлу
    listen(server_fd, 5); // ждём клиентов

    // изменено: вместо одного accept - массив сокетов для poll
    struct pollfd fds[N];
    fds[0].fd = server_fd; // [0] - слушающий сокет
    fds[0].events = POLLIN; // "можно читать" = кто-то подключается
    for (int i = 1; i < N; i++)
        fds[i].fd = -1; // остальные места свободны

    printf("[SERVER] started on %s\n", SOCKET_PATH);

    // главный цикл - сервер работает, пока не нажмут Ctrl+C
    while (1) {
        poll(fds, N, -1); // ждём событие на любом сокете

        // новый клиент подключается
        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, NULL, NULL); // принимаем клиента
            for (int i = 1; i < N; i++) {
                if (fds[i].fd == -1) { // нашли свободное место
                    fds[i].fd = client_fd;
                    fds[i].events = POLLIN; // ждём от него данных
                    printf("[SERVER] client connected (fd=%d)\n", client_fd);
                    break;
                }
            }
        }

        // проверяем всех клиентов
        for (int i = 1; i < N; i++) {
            if (fds[i].fd == -1 || !(fds[i].revents & POLLIN))
                continue; // места нет или данных нет

            char buf[1024];
            ssize_t n = read(fds[i].fd, buf, sizeof(buf)); // читаем от клиента

            if (n > 0) {
                for (ssize_t k = 0; k < n; k++)
                    buf[k] = toupper((unsigned char)buf[k]); // в верхний регистр
                write(1, buf, n); // изменено: печатаем на экран сервера
            } else { // read вернул 0 - клиент отключился
                printf("[SERVER] client %d disconnected\n", fds[i].fd);
                close(fds[i].fd); // закрываем его сокет
                fds[i].fd = -1; // освобождаем место
            }
        }
    }

    return 0; // сюда не дойдём - сервер работает до Ctrl+C
}
