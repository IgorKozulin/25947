#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <aio.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define LIMIT 100

static struct aiocb requests[LIMIT];
static unsigned char bytes[LIMIT];
static int clients[LIMIT];

static int start_read(int i)
{
    memset(&requests[i], 0, sizeof(requests[i]));
    requests[i].aio_fildes = clients[i];
    requests[i].aio_buf = &bytes[i];
    requests[i].aio_nbytes = 1;
    requests[i].aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    requests[i].aio_sigevent.sigev_signo = SIGRTMIN;
    return aio_read(&requests[i]);
}

int main(void)
{
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "socket"
    };
    struct timespec timeout = {0, 100000000};
    sigset_t signals;
    int server, client, flags, sig, error, i;
    ssize_t n;

    sigemptyset(&signals);
    sigaddset(&signals, SIGRTMIN);
    sigaddset(&signals, SIGINT);

    if (sigprocmask(SIG_BLOCK, &signals, NULL) == -1)
        return 1;

    for (i = 0; i < LIMIT; i++)
        clients[i] = -1;

    server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server == -1) {
        perror("socket");
        return 1;
    }

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server);
        return 1;
    }

    flags = fcntl(server, F_GETFL);
    if (listen(server, LIMIT) == -1 || flags == -1 ||
        fcntl(server, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("listen or fcntl");
        close(server);
        unlink(addr.sun_path);
        return 1;
    }

    for (;;) {
        while ((client = accept(server, NULL, NULL)) >= 0) {
            for (i = 0; i < LIMIT; i++)
                if (clients[i] == -1)
                    break;

            flags = fcntl(client, F_GETFL);
            if (i == LIMIT || flags == -1 ||
                fcntl(client, F_SETFL, flags & ~O_NONBLOCK) == -1) {
                close(client);
                continue;
            }

            clients[i] = client;
            if (start_read(i) == -1) {
                perror("aio_read");
                close(client);
                clients[i] = -1;
            }
        }

        if (errno != EAGAIN && errno != EWOULDBLOCK &&
            errno != EINTR) {
            perror("accept");
            break;
        }

        sig = sigtimedwait(&signals, NULL, &timeout);
        if (sig == SIGINT)
            break;
        if (sig != SIGRTMIN)
            continue;

        for (i = 0; i < LIMIT; i++) {
            if (clients[i] == -1)
                continue;

            error = aio_error(&requests[i]);
            if (error == EINPROGRESS)
                continue;

            n = aio_return(&requests[i]);
            if (error == 0 && n > 0) {
                putchar(toupper(bytes[i]));
                fflush(stdout);
                if (start_read(i) == 0)
                    continue;
                perror("aio_read");
            }

            close(clients[i]);
            clients[i] = -1;
        }
    }

    for (i = 0; i < LIMIT; i++) {
        if (clients[i] != -1) {
            aio_cancel(clients[i], &requests[i]);
            close(clients[i]);
        }
    }

    close(server);
    unlink(addr.sun_path);
    return 0;
}
