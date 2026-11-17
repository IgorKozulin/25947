#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios original;
static volatile sig_atomic_t interrupted = 0;

static void restore_terminal(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
}

static void signal_handler(int signal_number)
{
    interrupted = signal_number;
}

int prepare_terminal(void)
{
    struct termios settings;
    if (tcgetattr(STDIN_FILENO, &original) == -1) {
        perror("tcgetattr");
        return 1;
    }
    struct sigaction action = {0};
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    int signals[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
        if (sigaction(signals[i], &action, NULL) == -1) {
            perror("sigaction");
            return 1;
        }
    }
    settings = original;
    settings.c_lflag &= ~(ICANON | ECHO | IEXTEN);
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;
    if (atexit(restore_terminal) != 0) return 1;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) == -1) {
        perror("tcsetattr");
        return 1;
    }
    return 0;
}

int main(void)
{
    if (prepare_terminal() != 0) return 1;
    printf("Continue? (y/n): ");
    fflush(stdout);
    unsigned char answer;
    ssize_t result = -1;
    while (!interrupted) {
        result = read(STDIN_FILENO, &answer, 1);
        if (result == -1 && errno == EINTR) continue;
        break;
    }
    if (interrupted) return 128 + interrupted;
    if (result != 1) {
        if (result == -1) perror("read");
        return 1;
    }
    printf("\nAnswer: %c\n", answer);
    return 0;
}
