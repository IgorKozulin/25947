
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    struct sigaction action = {0};
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) == -1 ||
        sigaction(SIGQUIT, &action, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    unsigned long count = 0;
    for (;;) {
        int signal_number;
        int error = sigwait(&signals, &signal_number);
        if (error != 0) {
            fprintf(stderr, "sigwait: %s\n", strerror(error));
            return EXIT_FAILURE;
        }

        if (signal_number == SIGQUIT) {
            printf("\nBells: %lu\n", count);
            return fflush(stdout) == EOF ? EXIT_FAILURE : EXIT_SUCCESS;
        }

        if (putchar('\a') == EOF || fflush(stdout) == EOF) {
            perror("stdout");
            return EXIT_FAILURE;
        }
        ++count;
    }
}
