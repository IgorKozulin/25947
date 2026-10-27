#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios old_term;
static int rows = 1;

static void restore_terminal(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_term);
}

static void redraw(const char *buffer, size_t length)
{
    size_t i = 0;
    int col = 0;
    printf("\r\033[2K");
    while (--rows > 0)
        printf("\033[A\r\033[2K");
    rows = 1;
    while (i < length) {
        if (buffer[i] == ' ') {
            if (col == 40) {
                putchar('\n');
                rows++;
                col = 0;
            }
            putchar(' ');
            col++;
            i++;
        } else {
            size_t start = i;
            size_t word_length;
            while (i < length && buffer[i] != ' ')
                i++;
            word_length = i - start;
            if (col > 0 && col + word_length > 40) {
                putchar('\n');
                rows++;
                col = 0;
            }
            while (start < i) {
                if (col == 40) {
                    putchar('\n');
                    rows++;
                    col = 0;
                }
                putchar(buffer[start++]);
                col++;
            }
        }
    }
    fflush(stdout);
}

int main(void)
{
    struct termios term;
    char buffer[4096];
    size_t pos = 0;
    char c;
    char erase_char;
    char kill_char;
    if (tcgetattr(STDIN_FILENO, &old_term) == -1) {
        perror("tcgetattr");
        return 1;
    }
    erase_char = old_term.c_cc[VERASE];
    kill_char = old_term.c_cc[VKILL];
    term = old_term;
    term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1) {
        perror("tcsetattr");
        return 1;
    }
    atexit(restore_terminal);
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 4 && pos == 0)
            break;
        if (c == erase_char) {
            if (pos > 0)
                pos--;
            redraw(buffer, pos);
        } else if (c == kill_char) {
            pos = 0;
            redraw(buffer, pos);
        } else if (c == 23) {
            while (pos > 0 && buffer[pos - 1] == ' ')
                pos--;
            while (pos > 0 && buffer[pos - 1] != ' ')
                pos--;
            redraw(buffer, pos);
        } else if (c == '\n' || c == '\r') {
            putchar('\n');
            fflush(stdout);
            pos = 0;
            rows = 1;
        } else if (isprint((unsigned char)c)) {
            if (pos < sizeof(buffer)) {
                buffer[pos++] = c;
                redraw(buffer, pos);
            } else {
                putchar('\a');
                fflush(stdout);
            }
        } else {
            putchar('\a');
            fflush(stdout);
        }
    }
    putchar('\n');
    return 0;
}
