#define _POSIX_C_SOURCE 200112L

#include <ctype.h>        // toupper().
#include <errno.h>        // errno, EINTR.
#include <stddef.h>       // Размеры и системные типы.
#include <stdio.h>        // printf(), perror(), fflush().
#include <stdlib.h>       // Общие функции.
#include <string.h>       // memset(), strncpy().
#include <sys/socket.h>   // socket(), bind(), listen(), accept().
#include <sys/types.h>    // Системные типы.
#include <sys/un.h>       // struct sockaddr_un.
#include <unistd.h>       // read(), write(), close(), unlink().

// Путь к файлу Unix socket.
#define SOCKET_PATH "./socket"

// Записывает весь текст из buffer.
// write() может записать только часть текста, поэтому используем цикл.
static int write_all(int fd, const char *buffer, size_t length)
{
    // Пока не отправлены все символы.
    while (length > 0) {
        // Записываем текст в fd.
        ssize_t written = write(fd, buffer, length);

        // Если произошла ошибка записи.
        if (written == -1) {
            // Если запись прервана сигналом, пробуем ещё раз.
            if (errno == EINTR) {
                continue;
            }

            // Другая ошибка.
            return -1;
        }

        // Если ничего не записалось, сообщаем об ошибке.
        if (written == 0) {
            return -1;
        }

        // Переходим к ещё неотправленной части текста.
        buffer += written;

        // Уменьшаем количество символов для отправки.
        length -= (size_t)written;
    }

    // Весь текст успешно отправлен.
    return 0;
}

// Главная функция сервера.
int main(void)
{
    // Дескриптор socket, на котором сервер ждёт клиента.
    int server_fd;

    // Дескриптор соединения с конкретным подключившимся клиентом.
    int client_fd;

    // Структура с адресом Unix socket.
    struct sockaddr_un address;

    // Буфер для текста, который сервер получает от клиента.
    char buffer[1024];

    // Удаляем старый файл socket, если он остался после прошлого запуска.
    unlink(SOCKET_PATH);

    // Создаём Unix socket.
    // AF_UNIX — связь внутри одного компьютера.
    // SOCK_STREAM — потоковая надёжная передача данных.
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    // Проверяем ошибку создания socket.
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    // Очищаем структуру адреса.
    memset(&address, 0, sizeof(address));

    // Указываем тип адреса: Unix socket.
    address.sun_family = AF_UNIX;

    // Записываем путь к файлу socket.
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    // Привязываем socket к файлу ./socket.
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");

        // Закрываем socket при ошибке.
        close(server_fd);

        return 1;
    }

    // Переводим socket в режим ожидания подключений клиентов.
    if (listen(server_fd, 5) == -1) {
        perror("listen");

        // Закрываем socket.
        close(server_fd);

        // Удаляем файл socket.
        unlink(SOCKET_PATH);

        return 1;
    }

    // Сообщаем, что сервер запущен и ожидает клиента.
    printf("Server started. Waiting for a client on %s\n", SOCKET_PATH);

    // Сразу выводим сообщение на экран.
    fflush(stdout);

    // Ждём подключения клиента.
    // После подключения accept вернёт новый дескриптор client_fd.
    client_fd = accept(server_fd, NULL, NULL);

    // Проверяем ошибку подключения.
    if (client_fd == -1) {
        perror("accept");

        // Закрываем серверный socket.
        close(server_fd);

        // Удаляем файл socket.
        unlink(SOCKET_PATH);

        return 1;
    }

    // Сообщаем, что клиент подключился.
    printf("Client connected.\n");
    fflush(stdout);

    // Читаем текст от клиента, пока он не отключится.
    while (1) {
        // Получаем текст клиента в buffer.
        ssize_t count = read(client_fd, buffer, sizeof(buffer));

        // count == 0 означает, что клиент закрыл соединение.
        if (count == 0) {
            break;
        }

        // Проверяем ошибку чтения.
        if (count == -1) {
            // Если read был прерван сигналом, повторяем чтение.
            if (errno == EINTR) {
                continue;
            }

            perror("read");

            // Закрываем соединение с клиентом.
            close(client_fd);

            // Закрываем socket сервера.
            close(server_fd);

            // Удаляем файл socket.
            unlink(SOCKET_PATH);

            return 1;
        }

        // Преобразуем каждый символ в верхний регистр.
        for (ssize_t i = 0; i < count; i++) {
            buffer[i] = (char)toupper((unsigned char)buffer[i]);
        }

        // Печатаем преобразованный текст на экран сервера.
        if (write_all(STDOUT_FILENO, buffer, (size_t)count) == -1) {
            perror("write");

            // Закрываем соединение с клиентом.
            close(client_fd);

            // Закрываем socket сервера.
            close(server_fd);

            // Удаляем файл socket.
            unlink(SOCKET_PATH);

            return 1;
        }
    }

    // Клиент закрыл соединение.
    printf("Client disconnected. Server finished.\n");

    // Закрываем соединение с клиентом.
    close(client_fd);

    // Закрываем socket сервера.
    close(server_fd);

    // Удаляем файл socket.
    unlink(SOCKET_PATH);

    // Нормально завершаем сервер.
    return 0;
}
