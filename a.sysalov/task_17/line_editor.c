#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
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

#define WIDTH 40

static size_t screen_row = 0;

void redraw(const char *line, size_t length)
{
    printf("\r");
    if (screen_row > 0) printf("\033[%zuA", screen_row);
    printf("\033[J");
    size_t column = 0;
    screen_row = 0;
    for (size_t i = 0; i < length;) {
        size_t end = i;
        if (line[i] == ' ') {
            end++;
        } else {
            while (end < length && line[end] != ' ') end++;
            if (column > 0 && column + end - i > WIDTH) {
                printf("\n");
                screen_row++;
                column = 0;
            }
        }
        while (i < end) {
            if (column == WIDTH) {
                printf("\n");
                screen_row++;
                column = 0;
            }
            putchar(line[i++]);
            column++;
        }
    }
    fflush(stdout);
}

int main(void)
{
    if (prepare_terminal() != 0) return 1;
    char *line = NULL;
    size_t length = 0;
    size_t capacity = 0;
    unsigned char symbol;
    int status = 0;
    while (!interrupted) {
        ssize_t result = read(STDIN_FILENO, &symbol, 1);
        if (result == -1 && errno == EINTR) continue;
        if (result <= 0) {
            if (result == -1) {
                perror("read");
                status = 1;
            }
            break;
        }
        if (symbol == 4 && length == 0) break;
        if (symbol == original.c_cc[VERASE]) {
            if (length > 0) length--;
        } else if (symbol == original.c_cc[VKILL]) {
            length = 0;
        } else if (symbol == 23) {
            while (length > 0 && line[length - 1] == ' ') length--;
            while (length > 0 && line[length - 1] != ' ') length--;
        } else if (symbol == '\n' || symbol == '\r') {
            putchar('\n');
            fflush(stdout);
            length = 0;
            screen_row = 0;
            continue;
        } else if (isprint(symbol)) {
            if (length == capacity) {
                size_t new_capacity = capacity ? capacity * 2 : 128;
                char *new_line = realloc(line, new_capacity);
                if (new_line == NULL) {
                    perror("realloc");
                    status = 1;
                    break;
                }
                line = new_line;
                capacity = new_capacity;
            }
            line[length++] = (char)symbol;
        } else {
            putchar('\a');
            fflush(stdout);
            continue;
        }
        redraw(line, length);
    }
    free(line);
    putchar('\n');
    return interrupted ? 128 + interrupted : status;
}
