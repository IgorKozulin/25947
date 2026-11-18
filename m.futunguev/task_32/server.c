/*
Задание 32: AIO-сервер (aio_read / aio_error / aio_return) без poll.
добавлено: время жизни клиента растет с каждым подключением:
  1-й клиент — 5 с, 2-й — 7 с, 3-й — 9 с, 4-й — 11 с, ... (шаг +2 с).
Формула: lifetime = 5 + 2 * connect_counter (counter начинается с 0).
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <aio.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define N 10

#define LIFETIME_BASE 5   // ДОБАВЛЕНО: первый клиент — 5 секунд
#define LIFETIME_STEP 2   // ДОБАВЛЕНО: каждый следующий +2 секунды

struct client {
    int fd;
    struct aiocb cb;
    char byte;
    char line[1024];
    int len;
    time_t start;
    int lifetime;         // лимит ЭТОГО клиента
};

struct client clients[N];

static int connect_counter = 0;   // сколько клиентов уже подключилось

void arm_read(struct client *c) {
    memset(&c->cb, 0, sizeof(c->cb));
    c->cb.aio_fildes = c->fd;
    c->cb.aio_buf = &c->byte;
    c->cb.aio_nbytes = 1;
    c->cb.aio_sigevent.sigev_notify = SIGEV_NONE;
    aio_read(&c->cb);
}

void close_client(struct client *c, const char *reason) {
    if (c->len > 0) {
        write(1, c->line, c->len);
        write(1, "\n", 1);
    }
    printf("[SERVER] client %d %s (lived %lds, limit %ds)\n",
           c->fd, reason,
           (long)(time(NULL) - c->start),
           c->lifetime);
    close(c->fd);
    c->fd = -1;
    c->len = 0;
}

int main(void) {
    setbuf(stdout, NULL);

    unlink(SOCKET_PATH);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK);

    for (int i = 0; i < N; i++)
        clients[i].fd = -1;

    printf("[SERVER] AIO server started on %s\n", SOCKET_PATH);

    while (1) {
        // 1) принимаем всех новых клиентов
        int client_fd;
        while ((client_fd = accept(server_fd, NULL, NULL)) != -1) {
            fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) & ~O_NONBLOCK);

            int placed = 0;
            for (int i = 0; i < N; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd    = client_fd;
                    clients[i].len   = 0;
                    clients[i].start = time(NULL);

                    // добавлено: лимит растёт с каждым подключением
                    clients[i].lifetime = LIFETIME_BASE + LIFETIME_STEP * connect_counter;
                    printf("[SERVER] client connected (fd=%d, время жизни=%ds)\n",
                           client_fd, clients[i].lifetime);
                    connect_counter++;

                    arm_read(&clients[i]);
                    placed = 1;
                    break;
                }
            }

            if (!placed) {
                printf("[SERVER] no free slot, closing fd=%d\n", client_fd);
                close(client_fd);
            }
        }

        // 2) проверяем всех клиентов
        for (int i = 0; i < N; i++) {
            struct client *c = &clients[i];
            if (c->fd == -1)
                continue;

            // проверка индивидуального времени жизни
            if (time(NULL) - c->start >= c->lifetime) {
                close_client(c, "timeout");
                continue;
            }

            int err = aio_error(&c->cb);
            if (err == EINPROGRESS || err == EAGAIN)
                continue;

            ssize_t n = aio_return(&c->cb);

            if (err == 0 && n > 0) {
                c->line[c->len++] = toupper((unsigned char)c->byte);
                if (c->byte == '\n' || c->len == (int)sizeof(c->line)) {
                    write(1, c->line, c->len);
                    c->len = 0;
                }
                arm_read(c);
            } else {
                close_client(c, "disconnected");
            }
        }

        usleep(1000);
    }

    return 0;
}