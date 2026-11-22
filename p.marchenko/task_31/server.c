#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#define SOCK_PATH "./socket"
#define MAX_CLIENTS 10

/* Флаг для выхода из цикла по сигналу (для старых компиляторов без volatile sig_atomic_t) */
static int stop_requested = 0;

void sigint_handler(int sig) {
    (void)sig;
    stop_requested = 1;
}

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    int i, j;

    /* Настройка сигнала для корректного выхода по Ctrl+C */
    signal(SIGINT, sigint_handler);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); exit(1); }

    unlink(SOCK_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind"); exit(1);
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen"); exit(1);
    }

    printf("[СЕРВЕР] Запущен. Ожидание подключений на %s...\n", SOCK_PATH);
    printf("[СЕРВЕР] Нажмите Ctrl+C для остановки.\n");

    /* Инициализация pollfd */
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    for (i = 1; i <= MAX_CLIENTS; i++) {
        fds[i].fd = -1;
    }

    while (!stop_requested) {
        /* Ждем событий с таймаутом 1000 мс */
        int ret = poll(fds, nfds, 1000);
        if (ret == -1) {
            if (errno == EINTR) continue; // Прерван сигналом
            perror("poll"); break;
        }
        if (ret == 0) continue; // Таймаут

        for (i = 0; i < nfds; i++) {
            if (fds[i].fd == -1) continue;
            if (!(fds[i].revents & POLLIN)) continue;

            if (fds[i].fd == server_fd) {
                /* Новый клиент */
                client_fd = accept(server_fd, NULL, NULL);
                if (client_fd == -1) { perror("accept"); continue; }

                /* Ищем свободный слот */
                int added = 0;
                for (j = 1; j <= MAX_CLIENTS; j++) {
                    if (fds[j].fd == -1) {
                        fds[j].fd = client_fd;
                        fds[j].events = POLLIN;
                        if (j >= nfds) nfds = j + 1;
                        added = 1;
                        printf("[СЕРВЕР] Новый клиент подключен (fd=%d).\n", client_fd);
                        break;
                    }
                }
                if (!added) {
                    printf("[СЕРВЕР] Слишком много клиентов, отклонен.\n");
                    close(client_fd);
                }
            } else {
                /* Данные от клиента */
                char buffer[1024];
                ssize_t n = read(fds[i].fd, buffer, sizeof(buffer) - 1);
                
                if (n > 0) {
                    buffer[n] = '\0';
                    for (j = 0; j < n; j++)
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    printf("[Клиент %d]: %s", fds[i].fd, buffer);
                    fflush(stdout);
                } else {
                    /* Отключение или ошибка */
                    if (n == 0 || (n == -1 && errno != EINTR)) {
                        printf("[СЕРВЕР] Клиент %d отключился.\n", fds[i].fd);
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        
                        /* Оптимизация nfds (необязательно, но полезно) */
                        while (nfds > 1 && fds[nfds - 1].fd == -1) nfds--;
                    }
                }
            }
        }
    }

    printf("\n[СЕРВЕР] Завершение работы.\n");
    for (i = 0; i < nfds; i++) {
        if (fds[i].fd != -1) close(fds[i].fd);
    }
    close(server_fd);
    unlink(SOCK_PATH);
    return 0;
}
