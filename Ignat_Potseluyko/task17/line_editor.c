#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#define BUF_SIZE 4096
#define MAX_COL 40

static struct termios orig_termios;
static char buffer[BUF_SIZE];
static int pos = 0;
static int line_start = 0;
static unsigned char erase_char;
static unsigned char kill_char;

static void disable_raw_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enable_raw_mode(void)
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    erase_char = orig_termios.c_cc[VERASE];
    kill_char = orig_termios.c_cc[VKILL];
}

static void redraw(void)
{
    printf("\r\033[K");
    int col = 0;
    int i = 0;
    while (i < pos) {
        while (i < pos && buffer[i] == ' ') {
            if (col >= MAX_COL) {
                printf("\n\033[K");
                col = 0;
            }
            putchar(' ');
            col++;
            i++;
        }
        if (i >= pos) break;
        int word_start = i;
        while (i < pos && buffer[i] != ' ') i++;
        int word_len = i - word_start;
        if (col > 0 && col + word_len > MAX_COL) {
            printf("\n\033[K");
            col = 0;
        }
        if (word_len > MAX_COL) {
            for (int j = word_start; j < i; j++) {
                if (col >= MAX_COL) {
                    printf("\n\033[K");
                    col = 0;
                }
                putchar(buffer[j]);
                col++;
            }
        } else {
            for (int j = word_start; j < i; j++) {
                putchar(buffer[j]);
                col++;
            }
        }
    }
    fflush(stdout);
}

static void delete_last_word(void)
{
    int i = pos;
    while (i > 0 && buffer[i - 1] == ' ') i--;
    while (i > 0 && buffer[i - 1] != ' ') i--;
    pos = i;
}

int main(void)
{
    enable_raw_mode();
    printf("> ");
    fflush(stdout);

    unsigned char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 4) {
            if (pos == 0) {
                printf("\nExiting...\n");
                break;
            } else {
                printf("\a");
                fflush(stdout);
            }
        } else if (c == erase_char || c == 127 || c == 8) {
            if (pos > 0) {
                pos--;
                redraw();
            }
        } else if (c == kill_char || c == 21) {
            pos = 0;
            redraw();
        } else if (c == 23) {
            delete_last_word();
            redraw();
        } else if (c == '\n' || c == '\r') {
            printf("\n");
            buffer[pos] = '\0';
            printf("%s\n", buffer);
            pos = 0;
            line_start = 0;
            printf("> ");
            fflush(stdout);
        } else if (isprint(c)) {
            if (pos < BUF_SIZE - 1) {
                buffer[pos++] = c;
                redraw();
            }
        } else {
            printf("\a");
            fflush(stdout);
        }
    }
    return 0;
}

