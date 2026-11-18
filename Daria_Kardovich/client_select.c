#define _POSIX_C_SOURCE 200112L // Включает объявления POSIX функций

#include <errno.h> // Коды ошибок например EINTR
#include <signal.h> // Сигналы SIGPIPE
#include <stdio.h> // Функции printf и fgets
#include <string.h> // Функции memset strncpy strlen
#include <sys/socket.h> // Функции socket и connect
#include <sys/types.h> // Системные типы данных
#include <sys/un.h> // Структура sockaddr_un для Unix socket
#include <unistd.h> // Функции write и close

#define SOCKET_PATH "./socket31" // Путь к файлу сокета сервера

static int write_all(int fd, const char *buffer, size_t length) // Функция отправляет все символы из buffer в сокет
{ // Начало функции write_all
    while (length > 0) { // Повторяем отправку пока остались неотправленные символы
        ssize_t written = write(fd, buffer, length); // Отправляем данные в сокет и сохраняем число отправленных байтов

        if (written < 0) { // Проверяем произошла ли ошибка при отправке
            if (errno == EINTR) { // Проверяем прервала ли сигнал функцию write
                continue; // Повторяем попытку отправки
            } // Конец проверки EINTR

            return -1; // Возвращаем ошибку если отправить данные не получилось
        } // Конец проверки ошибки write

        if (written == 0) { // Проверяем что write не отправил ноль байтов
            return -1; // Возвращаем ошибку потому что данные дальше не отправляются
        } // Конец проверки нуля байтов

        buffer += written; // Передвигаем указатель после уже отправленных символов
        length -= (size_t)written; // Уменьшаем число оставшихся символов
    } // Конец цикла отправки

    return 0; // Сообщаем что все данные успешно отправлены
} // Конец функции write_all

int main(void) // Главная функция программы клиента
{ // Начало main
    int socket_fd; // Переменная для номера созданного сокета
    struct sockaddr_un address; // Структура с адресом Unix socket сервера
    char buffer[1024]; // Массив для введённой пользователем строки

    signal(SIGPIPE, SIG_IGN); // Не завершаем клиент если сервер неожиданно закрыл соединение

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0); // Создаём потоковый Unix socket

    if (socket_fd == -1) { // Проверяем удалось ли создать сокет
        perror("socket"); // Печатаем причину ошибки создания сокета
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки socket

    memset(&address, 0, sizeof(address)); // Заполняем всю структуру address нулями
    address.sun_family = AF_UNIX; // Указываем что это Unix domain socket
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1); // Записываем путь к сокету сервера

    if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) == -1) { // Подключаемся к серверу по указанному пути
        perror("connect"); // Печатаем причину ошибки подключения
        close(socket_fd); // Закрываем созданный сокет
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки connect

    printf("Connected. Empty line or Ctrl+D finishes input.\n"); // Сообщаем что подключение установлено

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) { // Считываем строки из терминала в buffer
        if (buffer[0] == '\n') { // Проверяем ввёл ли пользователь пустую строку
            break; // Выходим из цикла ввода
        } // Конец проверки пустой строки

        if (write_all(socket_fd, buffer, strlen(buffer)) == -1) { // Отправляем серверу все символы введённой строки
            perror("write"); // Печатаем причину ошибки отправки
            close(socket_fd); // Закрываем сокет
            return 1; // Завершаем программу с кодом ошибки
        } // Конец проверки write_all
    } // Конец цикла ввода строк

    close(socket_fd); // Закрываем соединение с сервером
    return 0; // Завершаем клиент без ошибки
} // Конец main
