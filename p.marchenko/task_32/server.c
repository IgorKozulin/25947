#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <aio.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "./socket"
#define MAX_CLIENTS 100

static volatile sig_atomic_t keep_running = 1;

void sigint_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

/* Структура для описания клиента */
struct client {
    int fd;
    struct aiocb cb;
    char byte;
    int active;
    int id;
};

/* Функция для постановки заявки на чтение 1 байта */
void arm_read(struct client *c) {
    memset(&c->cb, 0, sizeof(struct aiocb));
    c->cb.aio_fildes = c->fd;
    c->cb.aio_buf = &c->byte;
    c->cb.aio_nbytes = 1; /* Читаем по 1 байту для Solaris */
    c->cb.aio_offset = 0;
    c->cb.aio_sigevent.sigev_notify = SIGEV_NONE; /* Опрос через aio_error */

    if (aio_read(&c->cb) == -1) {
        /* На Solaris EAGAIN или EINPROGRESS - это норма */
        if (errno != EAGAIN && errno != EINPROGRESS) {
            perror("aio_read");
        }
    }
}

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    struct client clients[MAX_CLIENTS];
    int i, nclients = 0;

    signal(SIGINT, sigint_handler);

    /* Инициализация массива клиентов */
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].active = 0;
        clients[i].id = i + 1;
    }

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

    /* Слушающий сокет делаем неблокирующим, чтобы accept не тормозил */
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    printf("[СЕРВЕР] AIO-сервер запущен на %s\n", SOCK_PATH);
    printf("[СЕРВЕР] Ctrl+C для остановки.\n");

    while (keep_running) {
        /* 1. Проверяем новые подключения (в цикле, так как неблокирующий) */
        while ((client_fd = accept(server_fd, NULL, NULL)) != -1) {
            int slot = -1;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) { slot = i; break; }
            }
            if (slot != -1) {
                clients[slot].fd = client_fd;
                clients[slot].active = 1;
                nclients++;
                
                /* ВАЖНО: сбрасываем O_NONBLOCK для клиентского сокета (требование Solaris) */
                flags = fcntl(client_fd, F_GETFL, 0);
                fcntl(client_fd, F_SETFL, flags & ~O_NONBLOCK);

                printf("[СЕРВЕР] Клиент %d подключён (slot=%d fd=%d). Всего: %d\n",
                       clients[slot].id, slot, client_fd, nclients);
                
                arm_read(&clients[slot]); /* Ставим первую заявку на чтение */
            } else {
                close(client_fd); /* Нет свободных слотов */
            }
        }

        /* 2. Проверяем готовность данных от клиентов (опрос aio_error) */
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) continue;

            int err = aio_error(&clients[i].cb);
            
            if (err == EINPROGRESS || err == EAGAIN) {
                continue; /* Заявка ещё выполняется */
            } else if (err == 0) {
                /* Заявка завершена успешно */
                ssize_t n = aio_return(&clients[i].cb);
                
                if (n > 0) {
                    /* Обрабатываем 1 символ и выводим */
                    char ch = (char)toupper((unsigned char)clients[i].byte);
                    write(STDOUT_FILENO, &ch, 1);
                    arm_read(&clients[i]); /* Ставим следующую заявку */
                } else if (n == 0) {
                    /* EOF - клиент отключился */
                    printf("\n[СЕРВЕР] Клиент %d отключился (EOF). Осталось: %d\n",
                           clients[i].id, nclients - 1);
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    clients[i].active = 0;
                    nclients--;
                }
            } else if (err != ECANCELED) {
                /* Реальная ошибка */
                printf("\n[СЕРВЕР] Ошибка клиента %d: %s\n", clients[i].id, strerror(err));
                close(clients[i].fd);
                clients[i].fd = -1;
                clients[i].active = 0;
                nclients--;
            }
        }

        /* 3. Небольшая пауза, чтобы не грузить CPU на 100% */
        usleep(10000); /* 10 мс */
    }

    printf("\n[СЕРВЕР] Завершение работы...\n");
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            aio_cancel(clients[i].fd, &clients[i].cb);
            close(clients[i].fd);
        }
    }
    close(server_fd);
    unlink(SOCK_PATH);
    return 0;
}
