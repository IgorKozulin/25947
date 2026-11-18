#define _POSIX_C_SOURCE 200112L

#include <errno.h>          
#include <signal.h>     
#include <stdio.h>         
#include <stdlib.h>      
#include <string.h>         
#include <sys/socket.h>     
#include <sys/types.h>      
#include <sys/un.h>       
#include <unistd.h>        

// Путь к файлу Unix socket, созданному сервером.
#define SOCKET_PATH "./socket"

// Отправляет весь текст в socket.
// Нужна, потому что write() может отправить не весь текст за один вызов.
static int write_all(int fd, const char *buffer, size_t length)
{
    // Пока в buffer остаются неотправленные символы.
    while (length > 0) {
        // Отправляем текст через socket.
        ssize_t written = write(fd, buffer, length);

        // Если произошла ошибка.
        if (written == -1) {
            // Если write прервал сигнал, пробуем ещё раз.
            if (errno == EINTR) {
                continue;
            }

            // Возвращаем ошибку.
            return -1;
        }

        // Если ничего не записалось, считаем это ошибкой.
        if (written == 0) {
            return -1;
        }

        // Переходим к ещё неотправленной части текста.
        buffer += written;

        // Уменьшаем число символов, которые нужно отправить.
        length -= (size_t)written;
    }

    // Весь текст отправлен успешно.
    return 0;
}

// Главная функция клиента.
int main(void)
{
    // Дескриптор socket, через который клиент общается с сервером.
    int socket_fd;

    // Структура с адресом Unix socket.
    struct sockaddr_un address;

    // Буфер для текста, введённого пользователем.
    char buffer[1024];

    // Если сервер закроется раньше, клиент не завершится автоматически.
    // Вместо этого write() вернёт ошибку.
    signal(SIGPIPE, SIG_IGN);

    // Создаём Unix socket.
    // AF_UNIX — связь внутри одного компьютера.
    // SOCK_STREAM — надёжная потоковая передача данных.
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    // Проверяем ошибку создания socket.
    if (socket_fd == -1) {
        perror("socket");
        return 1;
    }

    // Очищаем структуру адреса.
    memset(&address, 0, sizeof(address));

    // Указываем тип адреса: Unix socket.
    address.sun_family = AF_UNIX;

    // Записываем путь к файлу socket сервера.
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    // Подключаемся к серверу через файл ./socket.
    if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("connect");

        // Закрываем socket при ошибке.
        close(socket_fd);

        return 1;
    }

    // Сообщаем об успешном подключении.
    printf("Connected to server.\n");

    // Объясняем, как закончить ввод.
    printf("Enter text. Empty line or Ctrl+D finishes input.\n");

    // Читаем строки, введённые пользователем.
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        // Если пользователь нажал Enter на пустой строке.
        if (buffer[0] == '\n') {
            // Заканчиваем ввод.
            break;
        }

        // Отправляем введённый текст серверу.
        if (write_all(socket_fd, buffer, strlen(buffer)) == -1) {
            perror("write");

            // Закрываем соединение при ошибке.
            close(socket_fd);

            return 1;
        }
    }

    // Закрываем соединение с сервером.
    // Сервер увидит конец данных и завершит работу.
    close(socket_fd);

    // Сообщаем о закрытии соединения.
    printf("Connection closed.\n");

    // Нормально завершаем клиента.
    return 0;
}
