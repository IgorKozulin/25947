#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>

#define SOCKET_PATH "./socket"

static int write_all(int fd, const char *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t n = write(fd, buf + total, count - total);
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += n;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    // [ИЗМЕНЕНО] Теперь требуем минимум 3 аргумента: имя и режим
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <имя> <count|file> [файл]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    const char *name = argv[1];
    const char *mode = argv[2];
    int sock_fd;
    struct sockaddr_un addr;

    signal(SIGPIPE, SIG_IGN);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect"); exit(EXIT_FAILURE);
    }

    printf("[%s] Подключен к серверу в режиме '%s'.\n", name, mode);

    // [ИЗМЕНЕНО] Ветвление логики в зависимости от выбранного режима
    if (strcmp(mode, "count") == 0) {
        // [ИЗМЕНЕНО] Сначала отправляем команду COUNT
        write_all(sock_fd, "COUNT\n", 6);
        
        // [ИЗМЕНЕНО] Отправляем текст, который сервер должен посчитать
        const char *text = "Hello, this is a test message for counting!";
        printf("[%s] Отправляю на подсчёт: %s\n", name, text);
        write_all(sock_fd, text, strlen(text));
        write_all(sock_fd, "\n", 1);
    } 
    else if (strcmp(mode, "file") == 0) {
        // [ИЗМЕНЕНО] Проверка, что передан путь к файлу
        if (argc < 4) {
            fprintf(stderr, "Для режима file нужен аргумент: путь к файлу\n");
            close(sock_fd); return EXIT_FAILURE;
        }

        // [ИЗМЕНЕНО] Сначала отправляем команду FILE
        write_all(sock_fd, "FILE\n", 5);

        // [ИЗМЕНЕНО] Клиент сам читает локальный файл и шлёт его содержимое
        FILE *fp = fopen(argv[3], "r");
        if (!fp) { perror("fopen"); close(sock_fd); return EXIT_FAILURE; }

        char buf[512];
        size_t n;
        printf("[%s] Отправляю содержимое файла '%s'...\n", name, argv[3]);
        
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
            write_all(sock_fd, buf, n);
            usleep(100000);
        }
        fclose(fp);
    } 
    else {
        // [ИЗМЕНЕНО] Обработка неизвестного режима
        fprintf(stderr, "Неизвестный режим: %s (используй count или file)\n", mode);
        close(sock_fd); return EXIT_FAILURE;
    }

    printf("[%s] Отправка завершена. Закрытие соединения.\n", name);
    close(sock_fd);
    return EXIT_SUCCESS;
}
