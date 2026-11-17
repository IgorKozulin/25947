
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

static volatile sig_atomic_t beep_count = 0;
static volatile sig_atomic_t finished = 0;

void handle_sigint(int sig)
{
    (void)sig;
    beep_count++;
    write(STDOUT_FILENO, "\a", 1);
}

void handle_sigquit(int sig)
{
    (void)sig;
    finished = 1;
}

int main(void)
{
    struct sigaction sa_int;
    struct sigaction sa_quit;
    sigset_t block_mask;
    sigset_t old_mask;

    memset(&sa_int, 0, sizeof(sa_int));
    memset(&sa_quit, 0, sizeof(sa_quit));

    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);

    sa_quit.sa_handler = handle_sigquit;
    sigemptyset(&sa_quit.sa_mask);

    if (sigaction(SIGINT, &sa_int, NULL) == -1 ||
        sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &block_mask, &old_mask) == -1) {
        perror("sigprocmask");
        return 1;
    }

    printf("Press Ctrl+C to beep, Ctrl+\\ to quit\n");
    fflush(stdout);

    while (!finished) {
        sigsuspend(&old_mask);
    }

    sigprocmask(SIG_SETMASK, &old_mask, NULL);

    printf("\nNumber of beeps: %d\n", (int)beep_count);

    return 0;
}

