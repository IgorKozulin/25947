#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>

#define SOCKET_PATH "./unix_socket"

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[1024];
    ssize_t bytes_read;

    // Игнорируем SIGPIPE на всякий случай, чтобы сервер не упал, если клиент резко отключится
    signal(SIGPIPE, SIG_IGN);

    // 1. Очистка: удаляем старый файл сокета, если он остался от прошлого запуска
    unlink(SOCKET_PATH);

    // 2. Создание сокета
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 3. Настройка адреса
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // 4. Привязка к файлу
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // 5. Прослушивание (очередь до 5 клиентов)
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен. Ожидание подключений на %s...\n", SOCKET_PATH);

    // 6. Принятие соединения (блокируется, пока клиент не подключится)
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("Клиент подключился!\n");

    // 7. Обработка данных
    while (1) {
        bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read == -1) {
            if (errno == EINTR) continue; // Если прервано сигналом — просто читаем дальше
            perror("read");
            break;
        }
        if (bytes_read == 0) {
            // 0 означает EOF — клиент закрыл соединение
            break;
        }

        buffer[bytes_read] = '\0'; // Для безопасной работы со строкой

        // Переводим в верхний регистр и выводим
        for (int i = 0; i < bytes_read; i++) {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }
        
        // Выводим строго в stdout сервера
        write(STDOUT_FILENO, buffer, bytes_read); 
    }

    printf("\nКлиент отключился. Завершение работы сервера.\n");

    // 8. Завершение: закрываем дескрипторы и удаляем файл сокета
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return EXIT_SUCCESS;
}
