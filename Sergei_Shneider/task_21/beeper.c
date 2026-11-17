#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t beep_count = 0;
static volatile sig_atomic_t finished = 0;

static void handle_signal(int sig)
{
    if (sig == SIGINT) {
        write(STDOUT_FILENO, "\a", 1);
        beep_count++;
    } else if (sig == SIGQUIT) {
        finished = 1;
    }
}

int main(void)
{
    struct sigaction action = {0};
    sigset_t signals, previous, waiting;

    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &signals, &previous) == -1) {
        perror("sigprocmask");
        return 1;
    }

    action.sa_handler = handle_signal;
    action.sa_mask = signals;
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }

    if (sigaction(SIGQUIT, &action, NULL) == -1) {
        perror("sigaction SIGQUIT");
        return 1;
    }

    waiting = previous;
    sigdelset(&waiting, SIGINT);
    sigdelset(&waiting, SIGQUIT);

    printf("Ctrl+C: beep; Ctrl+\\: finish\n");
    fflush(stdout);

    while (!finished) {
        sigsuspend(&waiting);
    }

    printf("\nBeeps: %d\n", (int)beep_count);
    return 0;
}
