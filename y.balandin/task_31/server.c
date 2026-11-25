#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 10

/* Флаг завершения по Ctrl+C */
static volatile sig_atomic_t running = 1;

static void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

int main(void) {
    int server_fd;
    struct sockaddr_un addr;
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;          // количество активных дескрипторов для poll
    char buffer[1024];

    /* Перехватываем Ctrl+C, чтобы красиво завершиться */
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    /* 1. Очистка старого файла сокета */
    unlink(SOCKET_PATH);

    /* 2. Создание сокета */
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    /* 3. Привязка к файлу */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind"); exit(EXIT_FAILURE);
    }

    /* 4. Прослушивание */
    if (listen(server_fd, 5) == -1) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    printf("[СЕРВЕР] Запущен. Ожидание подключений на %s...\n", SOCKET_PATH);
    printf("[СЕРВЕР] Нажмите Ctrl+C для остановки.\n");

    /* 5. Инициализация массива poll */
    for (int i = 0; i <= MAX_CLIENTS; i++) {
        fds[i].fd = -1;        // -1 = слот свободен
        fds[i].events = 0;
    }
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    /* 6. Главный цикл мультиплексирования */
    while (running) {
        int poll_res = poll(fds, nfds, 1000); // таймаут 1 сек для проверки running
        if (poll_res == -1) {
            if (errno == EINTR) continue;
            perror("poll"); break;
        }
        if (poll_res == 0) continue; // таймаут — просто проверяем running

        for (int i = 0; i < nfds; i++) {
            if (fds[i].fd == -1) continue;
            if (!(fds[i].revents & POLLIN)) continue;

            if (fds[i].fd == server_fd) {
                /* --- Новое подключение --- */
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd == -1) {
                    if (errno == EINTR) continue;
                    perror("accept"); continue;
                }

                /* Ищем свободный слот */
                int placed = 0;
                for (int j = 1; j <= MAX_CLIENTS; j++) {
                    if (fds[j].fd == -1) {
                        fds[j].fd = client_fd;
                        fds[j].events = POLLIN;
                        if (j >= nfds) nfds = j + 1;
                        printf("[СЕРВЕР] Новый клиент подключен (fd=%d). Всего: %d\n",
                               client_fd, nfds - 1);
                        placed = 1;
                        break;
                    }
                }
                if (!placed) {
                    printf("[СЕРВЕР] Слишком много клиентов, отклоняем fd=%d\n", client_fd);
                    close(client_fd);
                }
            } else {
                /* --- Данные от клиента --- */
                ssize_t bytes_read = read(fds[i].fd, buffer, sizeof(buffer) - 1);
                if (bytes_read == -1) {
                    if (errno == EINTR) continue;
                    perror("read");
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    continue;
                }
                if (bytes_read == 0) {
                    /* Клиент отключился */
                    printf("[СЕРВЕР] Клиент %d отключился.\n", fds[i].fd);
                    close(fds[i].fd);
                    fds[i].fd = -1;

                    /* Оптимизация: сдвигаем nfds, если хвост пуст */
                    while (nfds > 1 && fds[nfds - 1].fd == -1) nfds--;
                    printf("[СЕРВЕР] Осталось клиентов: %d\n", nfds - 1);
                    continue;
                }

                /* Преобразуем в верхний регистр и выводим */
                for (ssize_t j = 0; j < bytes_read; j++) {
                    buffer[j] = toupper((unsigned char)buffer[j]);
                }
                printf("[Клиент %d]: %.*s", fds[i].fd, (int)bytes_read, buffer);
                fflush(stdout);
            }
        }
    }

    /* 7. Завершение */
    printf("\n[СЕРВЕР] Завершение работы...\n");
    for (int i = 0; i < nfds; i++) {
        if (fds[i].fd != -1) close(fds[i].fd);
    }
    unlink(SOCKET_PATH);
    return EXIT_SUCCESS;
}
