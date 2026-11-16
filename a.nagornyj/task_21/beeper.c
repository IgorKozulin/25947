#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

static volatile sig_atomic_t interrupt_count;

static int
write_all(int fd, const char *text, size_t length)
{
    size_t written = 0;

    while (written < length) {
        ssize_t amount = write(fd, text + written, length - written);
        if (amount == -1 && errno == EINTR) {
            continue;
        }
        if (amount <= 0) {
            return -1;
        }
        written += (size_t)amount;
    }
    return 0;
}

static void
sigint_handler(int signal_number)
{
    static const char bell = '\a';
    (void)signal_number;

    if (interrupt_count < SIG_ATOMIC_MAX) {
        ++interrupt_count;
    }
    (void)write(STDOUT_FILENO, &bell, 1);
}

static void
sigquit_handler(int signal_number)
{
    static const char prefix[] = "Number of beeps: ";
    static const char suffix[] = "\nExiting on SIGQUIT.\n";
    char digits[32];
    size_t count = 0;
    uintmax_t value = (uintmax_t)interrupt_count;
    (void)signal_number;

    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0 && count < sizeof(digits));

    (void)write_all(STDOUT_FILENO, prefix, sizeof(prefix) - 1);
    while (count > 0) {
        --count;
        (void)write_all(STDOUT_FILENO, &digits[count], 1);
    }
    (void)write_all(STDOUT_FILENO, suffix, sizeof(suffix) - 1);
    _exit(EXIT_SUCCESS);
}

int
main(void)
{
    struct sigaction action;

    if (isatty(STDOUT_FILENO) &&
        printf("Beeper started. Press Ctrl-C for a beep; Ctrl-\\ to quit.\n") < 0) {
        perror("printf");
        return EXIT_FAILURE;
    }
    if (fflush(stdout) == EOF) {
        perror("fflush");
        return EXIT_FAILURE;
    }

    action.sa_handler = sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction(SIGINT)");
        return EXIT_FAILURE;
    }
    action.sa_handler = sigquit_handler;
    action.sa_flags = 0;
    if (sigaction(SIGQUIT, &action, NULL) == -1) {
        perror("sigaction(SIGQUIT)");
        return EXIT_FAILURE;
    }

    for (;;) {
        if (pause() == -1 && errno != EINTR) {
            perror("pause");
            return EXIT_FAILURE;
        }
    }
}
