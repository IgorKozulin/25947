#define __EXTENSIONS__ 1 // Включает расширения SunOS для FASYNC
#define _POSIX_C_SOURCE 200112L // Включает объявления POSIX функций

#include <ctype.h> // Функция toupper для верхнего регистра
#include <errno.h> // Коды ошибок например EAGAIN
#include <fcntl.h> // Функция fcntl и флаги сокета
#include <signal.h> // Сигналы SIGIO SIGINT и sigaction
#include <stdio.h> // Функции printf perror и fflush
#include <stdlib.h> // Стандартные функции библиотеки C
#include <string.h> // Функции memset и strncpy
#include <sys/file.h> // Флаг FASYNC для асинхронного ввода вывода
#include <sys/socket.h> // Функции socket bind listen accept
#include <sys/types.h> // Системные типы данных
#include <sys/un.h> // Структура sockaddr_un для Unix socket
#include <unistd.h> // Функции read write close pause и unlink

#define SOCKET_PATH "./socket32" // Путь к файлу сокета сервера
#define MAX_CLIENTS 64 // Максимальное число подключённых клиентов

static volatile sig_atomic_t event_happened = 1; // Флаг что на сокете произошло событие
static volatile sig_atomic_t server_running = 1; // Флаг что сервер продолжает работать

static void sigio_handler(int signal_number) // Обработчик сигнала о данных на сокете
{ // Начало функции sigio_handler
    (void)signal_number; // Убираем предупреждение о неиспользуемом аргументе
    event_happened = 1; // Сообщаем главному циклу что нужно проверить сокеты
} // Конец функции sigio_handler

static void sigint_handler(int signal_number) // Обработчик сигнала Ctrl+C
{ // Начало функции sigint_handler
    (void)signal_number; // Убираем предупреждение о неиспользуемом аргументе
    server_running = 0; // Сообщаем главному циклу что сервер надо остановить
} // Конец функции sigint_handler

static int enable_async_io(int fd) // Включает неблокирующий и асинхронный режим сокета
{ // Начало функции enable_async_io
    int flags; // Переменная для текущих флагов сокета

    flags = fcntl(fd, F_GETFL, 0); // Получаем текущие флаги сокета
    if (flags == -1) { // Проверяем произошла ли ошибка получения флагов
        return -1; // Возвращаем ошибку
    } // Конец проверки F_GETFL

    if (fcntl(fd, F_SETOWN, getpid()) == -1) { // Указываем что сигналы SIGIO получит этот процесс
        return -1; // Возвращаем ошибку
    } // Конец проверки F_SETOWN

    if (fcntl(fd, F_SETFL, flags | FNDELAY | FASYNC) == -1) { // Включаем неблокирующий и асинхронный режим
        return -1; // Возвращаем ошибку
    } // Конец проверки F_SETFL

    return 0; // Сообщаем что настройка сокета выполнена
} // Конец функции enable_async_io

static void close_client(int clients[], int index) // Закрывает сокет одного клиента
{ // Начало функции close_client
    close(clients[index]); // Закрываем сокет клиента
    clients[index] = -1; // Помечаем место клиента как свободное
} // Конец функции close_client

static void accept_new_clients(int server_fd, int clients[]) // Принимает всех новых клиентов
{ // Начало функции accept_new_clients
    while (1) { // Принимаем клиентов пока новые подключения существуют
        int client_fd = accept(server_fd, NULL, NULL); // Принимаем новое подключение и получаем сокет клиента

        if (client_fd == -1) { // Проверяем произошла ли ошибка accept
            if (errno == EWOULDBLOCK || errno == EAGAIN) { // Проверяем что новых клиентов больше нет
                return; // Выходим из функции приёма клиентов
            } // Конец проверки отсутствия клиентов

            perror("accept"); // Печатаем причину ошибки accept
            return; // Выходим из функции
        } // Конец проверки accept

        if (enable_async_io(client_fd) == -1) { // Настраиваем новый сокет клиента на асинхронный режим
            perror("enable_async_io for client"); // Печатаем причину ошибки настройки сокета клиента
            close(client_fd); // Закрываем неправильно настроенный сокет
            continue; // Пытаемся принять следующего клиента
        } // Конец проверки настройки клиента

        for (int i = 0; i < MAX_CLIENTS; i++) { // Ищем свободное место в массиве клиентов
            if (clients[i] == -1) { // Проверяем свободно ли текущее место
                clients[i] = client_fd; // Сохраняем сокет нового клиента
                printf("Client connected.\n"); // Печатаем сообщение о подключении
                fflush(stdout); // Сразу выводим сообщение на терминал
                client_fd = -1; // Сообщаем что сокет успешно сохранён
                break; // Прекращаем поиск свободного места
            } // Конец проверки свободного места
        } // Конец поиска свободного места

        if (client_fd != -1) { // Проверяем что свободного места для клиента не нашлось
            printf("Too many clients. Connection rejected.\n"); // Печатаем сообщение об отказе клиенту
            close(client_fd); // Закрываем лишнее соединение
        } // Конец проверки числа клиентов
    } // Конец цикла accept
} // Конец функции accept_new_clients

static void read_clients(int clients[]) // Читает данные от всех подключённых клиентов
{ // Начало функции read_clients
    char buffer[1024]; // Массив для текста от клиента

    for (int i = 0; i < MAX_CLIENTS; i++) { // Проходим по всем местам в массиве клиентов
        if (clients[i] == -1) { // Проверяем что в этом месте нет клиента
            continue; // Переходим к следующему месту
        } // Конец проверки свободного места

        while (1) { // Читаем данные клиента пока доступные данные не закончатся
            ssize_t count = read(clients[i], buffer, sizeof(buffer)); // Читаем текст из сокета клиента

            if (count > 0) { // Проверяем что от клиента получен текст
                for (ssize_t j = 0; j < count; j++) { // Проходим по всем полученным символам
                    buffer[j] = (char)toupper((unsigned char)buffer[j]); // Переводим текущий символ в верхний регистр
                } // Конец перевода текста

                if (write(STDOUT_FILENO, buffer, (size_t)count) == -1) { // Выводим обработанный текст в терминал сервера
                    perror("write"); // Печатаем причину ошибки вывода
                } // Конец проверки write

                continue; // Пытаемся прочитать следующую часть текста от того же клиента
            } // Конец проверки полученного текста

            if (count == 0) { // Проверяем что клиент закрыл соединение
                printf("Client disconnected.\n"); // Печатаем сообщение об отключении клиента
                close_client(clients, i); // Закрываем сокет и освобождаем место клиента
                break; // Прекращаем чтение этого клиента
            } // Конец проверки отключения клиента

            if (errno == EWOULDBLOCK || errno == EAGAIN) { // Проверяем что у клиента пока нет новых данных
                break; // Прекращаем чтение этого клиента
            } // Конец проверки отсутствия данных

            perror("read"); // Печатаем причину другой ошибки чтения
            close_client(clients, i); // Закрываем сокет клиента с ошибкой
            break; // Прекращаем чтение этого клиента
        } // Конец цикла чтения клиента
    } // Конец цикла проверки всех клиентов
} // Конец функции read_clients

int main(void) // Главная функция сервера
{ // Начало main
    int server_fd; // Дескриптор главного сокета сервера
    int clients[MAX_CLIENTS]; // Массив сокетов всех клиентов
    struct sockaddr_un address; // Структура с адресом Unix socket
    struct sigaction action_io; // Настройки обработчика SIGIO
    struct sigaction action_int; // Настройки обработчика SIGINT

    for (int i = 0; i < MAX_CLIENTS; i++) { // Проходим по всем местам массива клиентов
        clients[i] = -1; // Помечаем каждое место как свободное
    } // Конец заполнения массива клиентов

    memset(&action_io, 0, sizeof(action_io)); // Заполняем структуру настроек SIGIO нулями
    action_io.sa_handler = sigio_handler; // Назначаем функцию обработки SIGIO
    sigemptyset(&action_io.sa_mask); // Указываем что дополнительные сигналы не блокируются
    action_io.sa_flags = 0; // Не задаём дополнительные флаги обработчика

    if (sigaction(SIGIO, &action_io, NULL) == -1) { // Устанавливаем обработчик сигнала SIGIO
        perror("sigaction SIGIO"); // Печатаем причину ошибки настройки SIGIO
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки SIGIO

    memset(&action_int, 0, sizeof(action_int)); // Заполняем структуру настроек SIGINT нулями
    action_int.sa_handler = sigint_handler; // Назначаем функцию обработки Ctrl+C
    sigemptyset(&action_int.sa_mask); // Указываем что дополнительные сигналы не блокируются
    action_int.sa_flags = 0; // Не задаём дополнительные флаги обработчика

    if (sigaction(SIGINT, &action_int, NULL) == -1) { // Устанавливаем обработчик сигнала Ctrl+C
        perror("sigaction SIGINT"); // Печатаем причину ошибки настройки SIGINT
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки SIGINT

    unlink(SOCKET_PATH); // Удаляем старый файл сокета если он остался после прошлого запуска

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0); // Создаём потоковый Unix socket сервера
    if (server_fd == -1) { // Проверяем удалось ли создать сокет
        perror("socket"); // Печатаем причину ошибки socket
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки socket

    memset(&address, 0, sizeof(address)); // Заполняем структуру адреса нулями
    address.sun_family = AF_UNIX; // Указываем что используется Unix domain socket
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1); // Записываем путь к файлу сокета

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) { // Связываем сокет сервера с путём socket32
        perror("bind"); // Печатаем причину ошибки bind
        close(server_fd); // Закрываем сокет сервера
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки bind

    if (listen(server_fd, 10) == -1) { // Переводим серверный сокет в режим ожидания клиентов
        perror("listen"); // Печатаем причину ошибки listen
        close(server_fd); // Закрываем сокет сервера
        unlink(SOCKET_PATH); // Удаляем файл сокета
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки listen

    if (enable_async_io(server_fd) == -1) { // Включаем асинхронный режим серверного сокета
        perror("enable_async_io for server"); // Печатаем причину ошибки настройки сервера
        close(server_fd); // Закрываем сокет сервера
        unlink(SOCKET_PATH); // Удаляем файл сокета
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки настройки сервера

    printf("Server started. Socket: %s\n", SOCKET_PATH); // Печатаем путь к работающему серверному сокету
    printf("Start one or more clients. Ctrl+C stops server.\n"); // Печатаем способ остановки сервера
    fflush(stdout); // Сразу выводим оба сообщения на терминал

    while (server_running) { // Работаем пока пользователь не нажал Ctrl+C
        if (event_happened) { // Проверяем сообщил ли сигнал SIGIO о новом событии
            event_happened = 0; // Сбрасываем флаг события
            accept_new_clients(server_fd, clients); // Принимаем всех новых клиентов
            read_clients(clients); // Читаем текст от всех клиентов
        } // Конец проверки события

        pause(); // Останавливаем программу до нового сигнала
    } // Конец главного цикла сервера

    for (int i = 0; i < MAX_CLIENTS; i++) { // Проходим по всем клиентским сокетам
        if (clients[i] != -1) { // Проверяем что клиент подключён
            close(clients[i]); // Закрываем сокет клиента
        } // Конец проверки клиента
    } // Конец закрытия клиентов

    close(server_fd); // Закрываем главный сокет сервера
    unlink(SOCKET_PATH); // Удаляем файл socket32

    printf("\nServer finished.\n"); // Печатаем сообщение об остановке сервера
    return 0; // Завершаем программу без ошибки
} // Конец main
