
#define _POSIX_C_SOURCE 200112L // Включает объявления POSIX функций

#include <errno.h> // Коды ошибок например EINTR
#include <signal.h> // Сигнал SIGPIPE
#include <stdio.h> // Функции printf fgets и perror
#include <string.h> // Функции memset strncpy и strlen
#include <sys/socket.h> // Функции socket и connect
#include <sys/types.h> // Системные типы данных
#include <sys/un.h> // Структура sockaddr_un для Unix socket
#include <unistd.h> // Функции write и close

#define SOCKET_PATH "./socket32" // Путь к файлу сокета сервера

static int write_all(int fd, const char *buffer, size_t length) // Отправляет все символы из buffer в сокет
{ // Начало функции write_all
    while (length > 0) { // Повторяем отправку пока остаются неотправленные символы
        ssize_t written = write(fd, buffer, length); // Отправляем текст в сокет и получаем число отправленных байтов

        if (written == -1) { // Проверяем произошла ли ошибка отправки
            if (errno == EINTR) { // Проверяем прервала ли сигнал функцию write
                continue; // Повторяем попытку отправки
            } // Конец проверки EINTR

            return -1; // Возвращаем ошибку если отправить данные не получилось
        } // Конец проверки write

        buffer += written; // Передвигаем указатель после уже отправленных символов
        length -= (size_t)written; // Уменьшаем число неотправленных символов
    } // Конец цикла отправки

    return 0; // Сообщаем что весь текст отправлен
} // Конец функции write_all

int main(void) // Главная функция клиента
{ // Начало main
    int socket_fd; // Переменная для номера сокета клиента
    struct sockaddr_un address; // Структура с адресом Unix socket сервера
    char buffer[1024]; // Массив для введённого пользователем текста

    signal(SIGPIPE, SIG_IGN); // Не завершаем клиент если сервер неожиданно отключился

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0); // Создаём потоковый Unix socket клиента
    if (socket_fd == -1) { // Проверяем удалось ли создать сокет
        perror("socket"); // Печатаем причину ошибки socket
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки socket

    memset(&address, 0, sizeof(address)); // Заполняем структуру address нулями
    address.sun_family = AF_UNIX; // Указываем что используется Unix domain socket
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1); // Записываем путь к серверному сокету

    if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) == -1) { // Подключаем клиента к серверу
        perror("connect"); // Печатаем причину ошибки подключения
        close(socket_fd); // Закрываем сокет клиента
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки connect

    printf("Connected to server.\n"); // Сообщаем что соединение с сервером установлено
    printf("Enter text. Empty line finishes input.\n"); // Сообщаем как завершить ввод

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) { // Читаем строки из терминала в buffer
        if (buffer[0] == '\n') { // Проверяем ввёл ли пользователь пустую строку
            break; // Завершаем цикл ввода
        } // Конец проверки пустой строки

        if (write_all(socket_fd, buffer, strlen(buffer)) == -1) { // Отправляем введённую строку серверу
            perror("write"); // Печатаем причину ошибки отправки
            close(socket_fd); // Закрываем сокет клиента
            return 1; // Завершаем программу с кодом ошибки
        } // Конец проверки write_all
    } // Конец цикла ввода

    close(socket_fd); // Закрываем соединение с сервером
    printf("Connection closed.\n"); // Сообщаем что клиент отключился

    return 0; // Завершаем программу без ошибки
} // Конец main
