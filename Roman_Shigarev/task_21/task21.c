#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

static volatile sig_atomic_t beep_count = 0;
static volatile sig_atomic_t finished = 0;

static void handle_signal(int signo)
{
    int saved_errno = errno;

    if (signo == SIGINT) {
        write(STDOUT_FILENO, "\a", 1);
        beep_count++;
    } else if (signo == SIGQUIT) {
        finished = 1;
    }

    errno = saved_errno;
}

int main(void)
{
    sigset_t signals, wait_mask;

    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &signals, &wait_mask) == -1) {
        perror("sigprocmask");
        return 1;
    }

    struct sigaction action = {0};
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

    sigdelset(&wait_mask, SIGINT);
    sigdelset(&wait_mask, SIGQUIT);

    puts("Ctrl+C: beep; Ctrl+\\: finish");
    fflush(stdout);

    while (!finished) {
        if (sigsuspend(&wait_mask) == -1 && errno != EINTR) {
            perror("sigsuspend");
            return 1;
        }
    }

    printf("\nBeeps: %d\n", (int)beep_count);
    return 0;
}
