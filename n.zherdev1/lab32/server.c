#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define PATH "/tmp/task32.sock"
#define MAX_CLIENTS 32

void stop(int sig)
{
    (void)sig;
    unlink(PATH);
    _exit(0);
}

int make_async(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return -1;
    if (fcntl(fd, F_SETOWN, getpid()) == -1)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK | FASYNC);
}

int main(void)
{
    int clients[MAX_CLIENTS];
    int server, sig;
    char buffer[1024];
    struct sockaddr_un addr = {0};
    sigset_t signals;

    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i] = -1;

    sigemptyset(&signals);
    sigaddset(&signals, SIGIO);
    sigprocmask(SIG_BLOCK, &signals, NULL);

    server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server == -1) {
        perror("socket");
        return 1;
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, PATH);
    unlink(PATH);

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1 ||
        listen(server, 5) == -1 ||
        make_async(server) == -1) {
        perror("server");
        close(server);
        unlink(PATH);
        return 1;
    }

    signal(SIGINT, stop);
    puts("Server started");

    while (sigwait(&signals, &sig) == 0) {

        int fd;
        while ((fd = accept(server, NULL, NULL)) != -1) {
            int i;
            for (i = 0; i < MAX_CLIENTS && clients[i] != -1; i++);

            if (i < MAX_CLIENTS && make_async(fd) == 0)
                clients[i] = fd;
            else
                close(fd);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == -1)
                continue;

            ssize_t n;

            while ((n = read(clients[i], buffer, sizeof(buffer))) > 0) {
                for (ssize_t j = 0; j < n; j++)
                    buffer[j] = toupper((unsigned char)buffer[j]);

                write(STDOUT_FILENO, buffer, n);
            }

            if (n == 0 || (n < 0 && errno != EAGAIN &&
                           errno != EWOULDBLOCK && errno != EINTR)) {
                close(clients[i]);
                clients[i] = -1;
            }
        }
    }

    close(server);
    unlink(PATH);
    return 0;
}

