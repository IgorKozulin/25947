#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t beep_count = 0;

static void sigint_handler(int signal_number)
{
    (void)signal_number;

    beep_count++;
    write(STDOUT_FILENO, "\a", 1);
}

static void write_count(sig_atomic_t count)
{
    char buffer[64];
    char digits[32];
    size_t position = 0;
    size_t digit_count = 0;

    const char message[] = "\nBeep count: ";
    write(STDOUT_FILENO, message, sizeof(message) - 1);

    if (count == 0) {
        write(STDOUT_FILENO, "0", 1);
    } else {
        while (count > 0) {
            digits[digit_count++] = (char)('0' + count % 10);
            count /= 10;
        }

        while (digit_count > 0) {
            buffer[position++] = digits[--digit_count];
        }

        write(STDOUT_FILENO, buffer, position);
    }

    write(STDOUT_FILENO, "\nProgram finished by SIGQUIT.\n", 31);
}

static void sigquit_handler(int signal_number)
{
    (void)signal_number;

    write_count(beep_count);
    _exit(EXIT_SUCCESS);
}

int main(void)
{
    struct sigaction action_int;
    struct sigaction action_quit;

    action_int.sa_handler = sigint_handler;
    sigemptyset(&action_int.sa_mask);
    sigaddset(&action_int.sa_mask, SIGQUIT);
    action_int.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &action_int, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }

    action_quit.sa_handler = sigquit_handler;
    sigemptyset(&action_quit.sa_mask);
    sigaddset(&action_quit.sa_mask, SIGINT);
    action_quit.sa_flags = 0;

    if (sigaction(SIGQUIT, &action_quit, NULL) == -1) {
        perror("sigaction SIGQUIT");
        return 1;
    }

    printf("Beeper started.\n");
    printf("Ctrl+C: beep and increment counter.\n");
    printf("Ctrl+\\: print counter and exit.\n");

    while (1) {
        pause();
    }
}
