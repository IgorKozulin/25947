#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <aio.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 100

// [ИЗМЕНЕНО] Добавлены константы режимов
enum { MODE_NONE = 0, MODE_COUNT, MODE_FILE };

static volatile sig_atomic_t keep_running = 1;

static void sig_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

struct client {
    int fd;
    int id;
    int active;
    char byte;
    struct aiocb cb;
    // [ИЗМЕНЕНО] Новые поля для режимов и подсчёта
    int mode;               // текущий режим (NONE/COUNT/FILE)
    int char_count;         // счётчик символов для MODE_COUNT
    char cmd_buf[64];       // буфер для накопления команды ("COUNT\n" / "FILE\n")
    int cmd_len;            // сколько уже накоплено
};

static int arm_read(struct client *c) {
    memset(&c->cb, 0, sizeof(struct aiocb));
    c->cb.aio_fildes = c->fd;
    c->cb.aio_buf = &c->byte;
    c->cb.aio_nbytes = 1;
    c->cb.aio_offset = 0;
    c->cb.aio_sigevent.sigev_notify = SIGEV_NONE;

    if (aio_read(&c->cb) == -1) {
        if (errno == EAGAIN || errno == EINPROGRESS) return 0;
        perror("aio_read");
        return -1;
    }
    return 0;
}

int main(void) {
    int server_fd;
    struct sockaddr_un addr;
    struct client clients[MAX_CLIENTS];
    int active_count = 0;
    int next_id = 1;

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].active = 0;
    }

    unlink(SOCKET_PATH);
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) == -1) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    printf("[СЕРВЕР] AIO-сервер запущен на %s\n", SOCKET_PATH);
    printf("[СЕРВЕР] Ctrl+C для остановки.\n");

    while (keep_running) {
        int new_fd;
        while ((new_fd = accept(server_fd, NULL, NULL)) != -1) {
            int slot = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i].active) { slot = i; break; }
            }
            if (slot == -1) {
                close(new_fd);
                continue;
            }

            int cflags = fcntl(new_fd, F_GETFL, 0);
            fcntl(new_fd, F_SETFL, cflags & ~O_NONBLOCK);

            clients[slot].fd = new_fd;
            clients[slot].id = next_id++;
            clients[slot].active = 1;
            // [ИЗМЕНЕНО] Инициализация новых полей при подключении
            clients[slot].mode = MODE_NONE;
            clients[slot].char_count = 0;
            clients[slot].cmd_len = 0;
            active_count++;

            printf("[СЕРВЕР] Клиент %d подключён (fd=%d). Всего: %d\n",
                   clients[slot].id, new_fd, active_count);

            if (arm_read(&clients[slot]) == -1) {
                close(new_fd);
                clients[slot].fd = -1;
                clients[slot].active = 0;
                active_count--;
            }
        }

        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) continue;

            int err = aio_error(&clients[i].cb);
            if (err == EINPROGRESS || err == EAGAIN) continue;

            if (err != 0 && err != ECANCELED) {
                close(clients[i].fd);
                clients[i].fd = -1;
                clients[i].active = 0;
                active_count--;
                continue;
            }

            ssize_t bytes_read = aio_return(&clients[i].cb);

            if (bytes_read > 0) {
                char c = clients[i].byte;

                // [ИЗМЕНЕНО] Вся логика обработки байта зависит от режима
                if (clients[i].mode == MODE_NONE) {
                    // [ИЗМЕНЕНО] Пока режим не выбран — собираем команду по байту
                    if (c == '\n' || clients[i].cmd_len >= (int)sizeof(clients[i].cmd_buf) - 1) {
                        clients[i].cmd_buf[clients[i].cmd_len] = '\0';
                        
                        // [ИЗМЕНЕНО] Определяем режим по команде
                        if (strncmp(clients[i].cmd_buf, "COUNT", 5) == 0) {
                            clients[i].mode = MODE_COUNT;
                            char msg[64];
                            int l = snprintf(msg, sizeof(msg), "[СЕРВЕР] Клиент %d -> режим COUNT\n", clients[i].id);
                            write(STDOUT_FILENO, msg, l);
                        } else if (strncmp(clients[i].cmd_buf, "FILE", 4) == 0) {
                            clients[i].mode = MODE_FILE;
                            char msg[64];
                            int l = snprintf(msg, sizeof(msg), "[СЕРВЕР] Клиент %d -> режим FILE\n", clients[i].id);
                            write(STDOUT_FILENO, msg, l);
                        }
                        clients[i].cmd_len = 0;
                    } else {
                        clients[i].cmd_buf[clients[i].cmd_len++] = c;
                    }
                } 
                else if (clients[i].mode == MODE_COUNT) {
                    // [ИЗМЕНЕНО] В режиме COUNT просто считаем символы (без \n)
                    if (c != '\n') clients[i].char_count++;
                } 
                else if (clients[i].mode == MODE_FILE) {
                    // [ИЗМЕНЕНО] В режиме FILE выводим в верхнем регистре (как раньше)
                    char up = toupper((unsigned char)c);
                    write(STDOUT_FILENO, &up, 1);
                }

                if (arm_read(&clients[i]) == -1) {
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    clients[i].active = 0;
                    active_count--;
                }
            } 
            else if (bytes_read == 0) {
                // [ИЗМЕНЕНО] При отключении в режиме COUNT выводим итоговое число
                if (clients[i].mode == MODE_COUNT) {
                    char buf[128];
                    int l = snprintf(buf, sizeof(buf),
                        "\n[СЕРВЕР] Клиент %d: всего символов = %d\n",
                        clients[i].id, clients[i].char_count);
                    write(STDOUT_FILENO, buf, l);
                } else {
                    char buf[64];
                    int l = snprintf(buf, sizeof(buf),
                        "\n[СЕРВЕР] Клиент %d отключился (EOF).\n", clients[i].id);
                    write(STDOUT_FILENO, buf, l);
                }

                printf("[СЕРВЕР] Осталось: %d\n", active_count - 1);
                close(clients[i].fd);
                clients[i].fd = -1;
                clients[i].active = 0;
                active_count--;
            } 
            else {
                close(clients[i].fd);
                clients[i].fd = -1;
                clients[i].active = 0;
                active_count--;
            }
        }
        usleep(10000);
    }

    printf("\n[СЕРВЕР] Завершение работы...\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            aio_cancel(clients[i].fd, &clients[i].cb);
            close(clients[i].fd);
        }
    }
    close(server_fd);
    unlink(SOCKET_PATH);
    return EXIT_SUCCESS;
}
