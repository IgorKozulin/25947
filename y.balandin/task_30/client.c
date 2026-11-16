#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>

#define SOCKET_PATH "./unix_socket"

// Обертка для надежной записи (write может записать меньше байт, чем просили)
int write_all(int fd, const char *buf, size_t count) {
    size_t total_written = 0;
    while (total_written < count) {
        ssize_t n = write(fd, buf + total_written, count - total_written);
        if (n == -1) {
            if (errno == EINTR) continue; // Прервано сигналом — пробуем снова
            return -1; // Настоящая ошибка
        }
        total_written += n;
    }
    return 0;
}

int main(void) {
    int sock_fd;
    struct sockaddr_un addr;

    // Игнорируем SIGPIPE, чтобы при обрыве связи сервером клиент не падал, а получал ошибку EPIPE
    signal(SIGPIPE, SIG_IGN);

    const char *messages[] = {
        "Hello World!\n",
        "Solaris!!!\n",
        "Unix domain socket.\n"
    };

    // 1. Создание сокета
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. Настройка адреса
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // 3. Подключение к серверу
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Подключено к серверу. Отправка сообщений...\n");

    // 4. Отправка данных
    for (int i = 0; i < 3; i++) {
        printf("[CLIENT] Отправляю: %s", messages[i]);
        
        if (write_all(sock_fd, messages[i], strlen(messages[i])) == -1) {
            perror("write");
            break;
        }
        
        // Небольшая пауза для наглядности (опционально)
        usleep(100000); 
    }

    printf("[CLIENT] Все сообщения отправлены. Закрытие соединения...\n");

    // 5. Завершение
    close(sock_fd);
    
    return EXIT_SUCCESS;
}
