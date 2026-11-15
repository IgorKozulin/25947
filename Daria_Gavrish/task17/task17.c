
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define WIDTH 40
#define MAX_LINE 4096

static struct termios original_termios;
static char line[MAX_LINE];
static size_t length = 0;
static size_t cursor_row = 0;
static size_t screen_rows = 1;
static int terminal_active = 0;

static void restore_terminal(void)
{
    if (terminal_active) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
        terminal_active = 0;
    }
}

static void handle_signal(int sig)
{
    restore_terminal();
    signal(sig, SIG_DFL);
    raise(sig);
}

static void write_all(const char *data, size_t size)
{
    while (size > 0) {
        ssize_t count = write(STDOUT_FILENO, data, size);

        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }

            restore_terminal();
            exit(1);
        }

        if (count == 0) {
            restore_terminal();
            exit(1);
        }

        data += count;
        size -= (size_t)count;
    }
}

static void move_up(size_t rows)
{
    if (rows > 0) {
        char sequence[64];
        int count = snprintf(sequence, sizeof(sequence),
                             "\033[%zuA", rows);

        write_all(sequence, (size_t)count);
    }
}

static void redraw(void)
{
    write_all("\r", 1);
    move_up(cursor_row);
    write_all("\033[J", 3);

    size_t column = 0;
    size_t row = 0;
    size_t i = 0;

    while (i < length) {
        if (line[i] == ' ') {
            if (column == WIDTH) {
                write_all("\r\n", 2);
                row++;
                column = 0;
            }

            write_all(&line[i], 1);
            column++;
            i++;
            continue;
        }

        size_t start = i;

        while (i < length && line[i] != ' ') {
            i++;
        }

        size_t word_length = i - start;

        if (column > 0 &&
            column + word_length > WIDTH &&
            word_length <= WIDTH) {
            write_all("\r\n", 2);
            row++;
            column = 0;
        }

        for (size_t j = start; j < i; j++) {
            if (column == WIDTH) {
                write_all("\r\n", 2);
                row++;
                column = 0;
            }

            write_all(&line[j], 1);
            column++;
        }
    }

    if (column == WIDTH) {
        write_all("\r\n", 2);
        row++;
    }

    cursor_row = row;
    screen_rows = row + 1;
}

static void erase_character(void)
{
    if (length > 0) {
        length--;
        redraw();
    }
}

static void erase_line(void)
{
    length = 0;
    redraw();
}

static void erase_word(void)
{
    while (length > 0 && line[length - 1] == ' ') {
        length--;
    }

    while (length > 0 && line[length - 1] != ' ') {
        length--;
    }

    redraw();
}

int main(void)
{
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr, "Terminal required\n");
        return 1;
    }

    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        return 1;
    }

    struct termios settings = original_termios;

    settings.c_lflag &= ~(ECHO | ICANON);
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &settings) == -1) {
        perror("tcsetattr");
        return 1;
    }

    terminal_active = 1;
    atexit(restore_terminal);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGHUP, handle_signal);

    unsigned char erase = original_termios.c_cc[VERASE];
    unsigned char kill = original_termios.c_cc[VKILL];

    for (;;) {
        unsigned char ch;
        ssize_t count = read(STDIN_FILENO, &ch, 1);

        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("read");
            break;
        }

        if (count == 0) {
            break;
        }

        if (ch == erase) {
            erase_character();
        } else if (ch == kill) {
            erase_line();
        } else if (ch == 23) {
            erase_word();
        } else if (ch == 4) {
            if (length == 0) {
                write_all("\r\n", 2);
                break;
            }

            write_all("\a", 1);
        } else if (ch == '\r' || ch == '\n') {
            write_all("\r\n", 2);
            length = 0;
            cursor_row = 0;
            screen_rows = 1;
        } else if (ch >= 32 && ch <= 126) {
            if (length < MAX_LINE - 1) {
                line[length++] = (char)ch;
                redraw();
            } else {
                write_all("\a", 1);
            }
        } else {
            write_all("\a", 1);
        }
    }

    return 0;
}

