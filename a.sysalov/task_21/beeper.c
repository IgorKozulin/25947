#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }

    printf("SIGINT: beep. SIGQUIT: print count and exit.\n");
    fflush(stdout);
    unsigned long count = 0;

    while (1) {
        int signal_number;
        int error = sigwait(&signals, &signal_number);
        if (error != 0) {
            fprintf(stderr, "sigwait: %s\n", strerror(error));
            return 1;
        }
        if (signal_number == SIGQUIT) {
            printf("\nBeep count: %lu\n", count);
            return 0;
        }
        if (putchar('\a') == EOF || fflush(stdout) == EOF) {
            perror("stdout");
            return 1;
        }
        count++;
    }
}
