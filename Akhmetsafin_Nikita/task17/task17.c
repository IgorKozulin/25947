
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

enum { LINE_WIDTH = 40, CTRL_D = 4, CTRL_W = 23 };

static struct termios original;

static void restore_on_signal(int signal_number)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    _exit(128 + signal_number);
}

static void erase_to(size_t *length, size_t position)
{
    while (*length > position) {
        fputs("\b \b", stdout);
        --*length;
    }
}

static size_t word_start(const unsigned char line[], size_t length)
{
    while (length > 0 && line[length - 1] != ' ') {
        --length;
    }
    return length;
}

int main(void)
{
    if (tcgetattr(STDIN_FILENO, &original) == -1) {
        perror("tcgetattr");
        return EXIT_FAILURE;
    }

    struct sigaction action = {0};
    action.sa_handler = restore_on_signal;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGHUP, &action, NULL) == -1 ||
        sigaction(SIGINT, &action, NULL) == -1 ||
        sigaction(SIGQUIT, &action, NULL) == -1 ||
        sigaction(SIGTERM, &action, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    long disabled = fpathconf(STDIN_FILENO, _PC_VDISABLE);
    struct termios editing = original;
    editing.c_lflag &= ~(ICANON | ECHO | ECHONL | ISIG | IEXTEN);
    editing.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR | IGNCR);
    editing.c_cc[VMIN] = 1;
    editing.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &editing) == -1) {
        perror("tcsetattr");
        return EXIT_FAILURE;
    }

    unsigned char line[LINE_WIDTH];
    size_t length = 0;
    int input;
    int result = EXIT_SUCCESS;

    while ((input = getchar()) != EOF) {
        if (input == CTRL_D && length == 0) {
            break;
        }
        if (input == original.c_cc[VERASE] && input != disabled) {
            if (length > 0) {
                erase_to(&length, length - 1);
            }
        } else if (input == original.c_cc[VKILL] && input != disabled) {
            erase_to(&length, 0);
        } else if (input == CTRL_W) {
            size_t end = length;
            while (end > 0 && line[end - 1] == ' ') {
                --end;
            }
            erase_to(&length, word_start(line, end));
        } else if (!isprint(input)) {
            putchar('\a');
        } else {
            if (length == LINE_WIDTH) {
                size_t start = word_start(line, length);
                if (input != ' ' && start > 0 && start < length) {
                    size_t end = length;
                    erase_to(&length, start);
                    putchar('\n');
                    length = 0;
                    for (size_t i = start; i < end; ++i) {
                        putchar(line[i]);
                        line[length++] = line[i];
                    }
                } else {
                    putchar('\n');
                    length = 0;
                }
            }
            line[length++] = (unsigned char)input;
            putchar(input);
        }

        if (fflush(stdout) == EOF) {
            perror("stdout");
            result = EXIT_FAILURE;
            break;
        }
    }

    if (ferror(stdin)) {
        perror("stdin");
        result = EXIT_FAILURE;
    }
    if (tcsetattr(STDIN_FILENO, TCSANOW, &original) == -1) {
        perror("tcsetattr");
        result = EXIT_FAILURE;
    }
    return result;
}
