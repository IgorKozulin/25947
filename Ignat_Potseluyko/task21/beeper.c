#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static volatile sig_atomic_t beep_count = 0;

static void sigint_handler(int sig)
{
    (void)sig;
    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

static void sigquit_handler(int sig)
{
    (void)sig;
    char buf[128];
    int n = snprintf(buf, sizeof(buf),
                     "\nВсего прозвучало сигналов: %d\n"
                     "Программа завершена по SIGQUIT\n",
                     (int)beep_count);
    write(STDOUT_FILENO, buf, (size_t)n);
    _exit(EXIT_SUCCESS);
}

int main(void)
{
    struct sigaction sa_int, sa_quit;

    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    memset(&sa_quit, 0, sizeof(sa_quit));
    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;
    sigaction(SIGQUIT, &sa_quit, NULL);

    while (1)
        pause();

    return 0;
}
