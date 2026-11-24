#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
static size_t screen_column = 0;
size_t cursor = 0;

void next_position(const char *line, size_t length, size_t index,
                   size_t *row, size_t *column)
{
    if (index == length) return;
    if (line[index] != ' ' && (index == 0 || line[index - 1] == ' ')) {
        size_t end = index;
        while (end < length && line[end] != ' ') end++;
        if (*column > 0 && *column + end - index > WIDTH) {
            (*row)++;
            *column = 0;
        }
    }
    if (*column == WIDTH) {
        (*row)++;
        *column = 0;
    }
}

size_t move_vertical(const char *line, size_t length, size_t cursor, int down)
{
    if (!down && screen_row == 0) return cursor;
    size_t target_row = down ? screen_row + 1 : screen_row - 1;
    size_t row = 0;
    size_t column = 0;
    size_t result = cursor;

    for (size_t i = 0; i <= length; i++) {
        next_position(line, length, i, &row, &column);
        if (row > target_row) break;
        if (row == target_row && column <= screen_column) result = i;
        column++;
    }
    return result;
}

void redraw(const char *line, size_t length, size_t cursor)
{
    printf("\r");
    if (screen_row > 0) printf("\033[%zuA", screen_row);
    printf("\033[J");
    size_t column = 0;
    size_t row = 0;
    size_t cursor_row = 0;
    size_t cursor_column = 0;
    for (size_t i = 0; i <= length; i++) {
        size_t previous_row = row;
        next_position(line, length, i, &row, &column);
        if (row > previous_row) putchar('\n');
        if (i == cursor) {
            cursor_row = row;
            cursor_column = column;
        }
        if (i < length) {
            putchar(line[i]);
            column++;
        }
    }
    printf("\r");
    if (row > cursor_row) printf("\033[%zuA", row - cursor_row);
    if (cursor_column > 0) printf("\033[%zuC", cursor_column);
    screen_row = cursor_row;
    screen_column = cursor_column;
    fflush(stdout);
}

int main(void)
{
    if (prepare_terminal() != 0) return 1;
    char *line = NULL;
    size_t length = 0;
    size_t capacity = 0;
    size_t cursor = 0;
    int replace_mode = 0;
    int escape = 0;
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
        if (symbol == 16) {
            replace_mode = !replace_mode;
            escape = 0;
            if (!replace_mode) cursor = length;
            redraw(line, length, cursor);
            continue;
        }
        if (symbol == 27) {
            escape = 1;
            continue;
        }
        if (escape == 1) {
            escape = symbol == '[' || symbol == 'O' ? 2 : 0;
            if (escape) continue;
        } else if (escape == 2) {
            escape = 0;
            if (replace_mode && symbol == 'D' && cursor > 0) {
                cursor--;
            } else if (replace_mode && symbol == 'C' && cursor < length) {
                cursor++;
            } else if (replace_mode && (symbol == 'A' || symbol == 'B')) {
                cursor = move_vertical(line, length, cursor, symbol == 'B');
            } else {
                putchar('\a');
            }
            redraw(line, length, cursor);
            continue;
        }
        if (symbol == 4 && length == 0) break;
        if (symbol == original.c_cc[VERASE]) {
            if (cursor > 0) {
                memmove(line + cursor - 1, line + cursor, length - cursor);
                cursor--;
                length--;
            }
        } else if (symbol == original.c_cc[VKILL]) {
            length = 0;
            cursor = 0;
        } else if (symbol == 23) {
            size_t start = cursor;
            while (start > 0 && line[start - 1] == ' ') start--;
            while (start > 0 && line[start - 1] != ' ') start--;
            if (start < cursor) {
                memmove(line + start, line + cursor, length - cursor);
                length -= cursor - start;
                cursor = start;
            }
        } else if (symbol == '\n' || symbol == '\r') {
            redraw(line, length, length);
            putchar('\n');
            fflush(stdout);
            length = 0;
            cursor = 0;
            screen_row = 0;
            continue;
        } else if (isprint(symbol)) {
            if (cursor == length && length == capacity) {
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
            line[cursor] = (char)symbol;
            if (cursor == length) length++;
            cursor++;
        } else {
            putchar('\a');
            fflush(stdout);
            continue;
        }
        redraw(line, length, cursor);
    }
    free(line);
    putchar('\n');
    return interrupted ? 128 + interrupted : status;
}
// insert