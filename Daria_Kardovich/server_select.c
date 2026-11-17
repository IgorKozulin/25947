#define _POSIX_C_SOURCE 200112L // Включает объявления POSIX функций

#include <ctype.h> // Функция toupper для перевода символов в верхний регистр
#include <errno.h> // Коды ошибок например EINTR
#include <signal.h> // Сигналы SIGINT и sigaction
#include <stdio.h> // Функции printf fprintf perror и fflush
#include <string.h> // Функции memset и strncpy
#include <sys/select.h> // Тип fd_set и функции FD_SET FD_ISSET
#include <sys/socket.h> // Функции socket bind listen accept
#include <sys/types.h> // Системные типы данных
#include <sys/un.h> // Структура sockaddr_un для Unix socket
#include <unistd.h> // Функции read write close и unlink

#define SOCKET_PATH "./socket31" // Путь к файлу сокета сервера

static volatile sig_atomic_t stop_server; // Флаг остановки сервера после Ctrl+C

static void stop_handler(int signal_number) // Обработчик сигнала Ctrl+C
{ // Начало функции stop_handler
    (void)signal_number; // Убираем предупреждение о неиспользуемом аргументе
    stop_server = 1; // Сообщаем основному циклу что сервер надо остановить
} // Конец функции stop_handler

static int write_all(int fd, const char *buffer, size_t length) // Функция выводит все символы из buffer
{ // Начало функции write_all
    while (length > 0) { // Повторяем вывод пока не выведены все символы
        ssize_t written = write(fd, buffer, length); // Пытаемся вывести данные и сохраняем число выведенных байтов

        if (written < 0) { // Проверяем произошла ли ошибка при write
            if (errno == EINTR) { // Проверяем прервала ли сигнал функцию write
                continue; // Повторяем попытку вывода
            } // Конец проверки EINTR

            return -1; // Возвращаем ошибку если вывести данные не получилось
        } // Конец проверки ошибки write

        if (written == 0) { // Проверяем что write не вывел ноль байтов
            return -1; // Возвращаем ошибку потому что вывод остановился
        } // Конец проверки нуля байтов

        buffer += written; // Передвигаем указатель после уже выведенных символов
        length -= (size_t)written; // Уменьшаем число оставшихся символов
    } // Конец цикла вывода

    return 0; // Сообщаем что все данные выведены
} // Конец функции write_all

int main(void) // Главная функция сервера
{ // Начало main
    int server_fd; // Дескриптор главного сокета сервера
    int max_fd; // Наибольший номер дескриптора для select
    fd_set master_set; // Набор всех сокетов которые отслеживает сервер
    struct sockaddr_un address; // Структура с адресом Unix socket
    struct sigaction action; // Структура с настройками обработчика сигнала

    memset(&action, 0, sizeof(action)); // Заполняем структуру action нулями
    action.sa_handler = stop_handler; // Назначаем обработчик Ctrl+C
    sigemptyset(&action.sa_mask); // Указываем что обработчик не блокирует дополнительные сигналы

    if (sigaction(SIGINT, &action, NULL) == -1) { // Устанавливаем обработчик сигнала Ctrl+C
        perror("sigaction"); // Печатаем причину ошибки настройки сигнала
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки sigaction

    unlink(SOCKET_PATH); // Удаляем старый файл сокета если он остался после прошлого запуска

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0); // Создаём потоковый Unix socket сервера

    if (server_fd == -1) { // Проверяем удалось ли создать сокет
        perror("socket"); // Печатаем причину ошибки создания сокета
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки socket

    memset(&address, 0, sizeof(address)); // Заполняем структуру адреса нулями
    address.sun_family = AF_UNIX; // Указываем что используется Unix domain socket
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1); // Записываем путь к файлу сокета

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) { // Связываем сокет сервера с путём socket31
        perror("bind"); // Печатаем причину ошибки bind
        close(server_fd); // Закрываем сокет сервера
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки bind

    if (listen(server_fd, 5) == -1) { // Переводим сокет в режим ожидания клиентов
        perror("listen"); // Печатаем причину ошибки listen
        close(server_fd); // Закрываем сокет сервера
        unlink(SOCKET_PATH); // Удаляем файл сокета
        return 1; // Завершаем программу с кодом ошибки
    } // Конец проверки listen

    FD_ZERO(&master_set); // Очищаем набор отслеживаемых сокетов
    FD_SET(server_fd, &master_set); // Добавляем главный сокет сервера в набор
    max_fd = server_fd; // Пока наибольший дескриптор это сокет сервера

    printf("Server started. Ctrl+C stops server.\n"); // Печатаем сообщение о запуске сервера
    fflush(stdout); // Сразу выводим сообщение на экран

    while (!stop_server) { // Работаем пока пользователь не нажал Ctrl+C
        fd_set ready_set = master_set; // Копируем все сокеты в набор для проверки готовности
        int ready = select(max_fd + 1, &ready_set, NULL, NULL, NULL); // Ждём пока данные появятся в одном из сокетов

        if (ready == -1) { // Проверяем произошла ли ошибка select
            if (errno == EINTR) { // Проверяем прервала ли сигнал функцию select
                continue; // Возвращаемся к началу цикла
            } // Конец проверки EINTR

            perror("select"); // Печатаем причину ошибки select
            break; // Завершаем главный цикл сервера
        } // Конец проверки select

        if (FD_ISSET(server_fd, &ready_set)) { // Проверяем хочет ли новый клиент подключиться
            int client_fd = accept(server_fd, NULL, NULL); // Принимаем подключение нового клиента

            if (client_fd == -1) { // Проверяем произошла ли ошибка accept
                if (errno != EINTR) { // Проверяем что ошибка не связана с сигналом
                    perror("accept"); // Печатаем причину ошибки accept
                } // Конец проверки ошибки accept
            } else if (client_fd >= FD_SETSIZE) { // Проверяем помещается ли клиентский сокет в fd_set
                fprintf(stderr, "Too many clients\n"); // Печатаем сообщение о слишком большом числе клиентов
                close(client_fd); // Закрываем лишнее соединение
            } else { // Выполняем код для корректного нового клиента
                FD_SET(client_fd, &master_set); // Добавляем сокет клиента в набор отслеживаемых сокетов

                if (client_fd > max_fd) { // Проверяем стал ли новый сокет наибольшим
                    max_fd = client_fd; // Сохраняем новый наибольший дескриптор
                } // Конец проверки max_fd

                printf("Client connected.\n"); // Печатаем сообщение о новом клиенте
                fflush(stdout); // Сразу выводим сообщение на экран
            } // Конец обработки нового клиента
        } // Конец проверки подключения клиента

        for (int fd = 0; fd <= max_fd; ++fd) { // Проверяем все возможные дескрипторы
            char buffer[1024]; // Создаём массив для текста от клиента
            ssize_t count; // Переменная для числа прочитанных байтов

            if (fd == server_fd || !FD_ISSET(fd, &ready_set)) { // Пропускаем серверный сокет и сокеты без данных
                continue; // Переходим к следующему дескриптору
            } // Конец проверки сокета

            count = read(fd, buffer, sizeof(buffer)); // Читаем текст из сокета клиента

            if (count == 0) { // Проверяем отключился ли клиент
                close(fd); // Закрываем сокет отключившегося клиента
                FD_CLR(fd, &master_set); // Удаляем сокет клиента из набора
                printf("Client disconnected.\n"); // Печатаем сообщение об отключении клиента
                fflush(stdout); // Сразу выводим сообщение на экран
                continue; // Переходим к следующему сокету
            } // Конец проверки отключения клиента

            if (count == -1) { // Проверяем произошла ли ошибка при чтении
                if (errno != EINTR) { // Проверяем что ошибка не вызвана сигналом
                    perror("read"); // Печатаем причину ошибки чтения
                } // Конец проверки ошибки read

                continue; // Переходим к следующему сокету
            } // Конец проверки read

            for (ssize_t i = 0; i < count; ++i) { // Проходим по всем полученным символам
                buffer[i] = (char)toupper((unsigned char)buffer[i]); // Переводим текущий символ в верхний регистр
            } // Конец перевода текста

            if (write_all(STDOUT_FILENO, buffer, (size_t)count) == -1) { // Выводим обработанный текст в терминал сервера
                perror("write"); // Печатаем причину ошибки вывода
                stop_server = 1; // Сообщаем что сервер надо остановить
                break; // Выходим из цикла проверки клиентов
            } // Конец проверки write_all
        } // Конец цикла проверки сокетов
    } // Конец главного цикла сервера

    for (int fd = 0; fd <= max_fd; ++fd) { // Проходим по всем возможным дескрипторам
        if (FD_ISSET(fd, &master_set)) { // Проверяем находится ли дескриптор в наборе
            close(fd); // Закрываем сокет сервера или клиента
        } // Конец проверки сокета
    } // Конец закрытия сокетов

    unlink(SOCKET_PATH); // Удаляем файл socket31 после остановки сервера
    return 0; // Завершаем программу без ошибки
} // Конец main
