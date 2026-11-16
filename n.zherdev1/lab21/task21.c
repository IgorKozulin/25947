#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t count = 0;

// SIGINT: пищим и увеличиваем счётчик.
void on_int(int sig)
{
    (void)sig;

    count++;
    write(STDOUT_FILENO, "\a", 1);
}

// SIGQUIT: выводим количество сигналов и завершаемся.
void on_quit(int sig)
{
    (void)sig;

    char buffer[64];
    int n = snprintf(buffer, sizeof(buffer),
                     "\nSignals: %d\n", (int)count);

    write(STDOUT_FILENO, buffer, n);
    _exit(0);
}

int main(void)
{
    signal(SIGINT, on_int);
    signal(SIGQUIT, on_quit);

    // Бесконечно ждём сигналы.
    while (1)
        pause();

    return 0;
}
