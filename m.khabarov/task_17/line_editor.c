#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#define MAXLEN 40
#define BUFSIZE 4096
#define PROMPT_COLS 2

static struct termios orig_termios;
static int raw_enabled = 0;

static void disable_raw_mode(void) {
    if (raw_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_enabled = 0;
    }
}

static void enable_raw_mode(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
    raw_enabled = 1;
    atexit(disable_raw_mode);
}

static void redraw(const char *buf, int len) {
    int col = PROMPT_COLS;
    int i = 0;

    printf("\r\033[K> ");

    while (i < len) {
        int word_start = i;
        while (i < len && !isspace((unsigned char)buf[i]))
            i++;
        int word_len = i - word_start;

        int sp_start = i;
        while (i < len && isspace((unsigned char)buf[i]))
            i++;
        int sp_len = i - sp_start;

        if (word_len > 0 && col > PROMPT_COLS && col + word_len > MAXLEN) {
            printf("\n\033[K> ");
            col = PROMPT_COLS;
        }

        fwrite(buf + word_start, 1, word_len, stdout);
        col += word_len;

        for (int k = 0; k < sp_len; k++) {
            if (col >= MAXLEN) {
                printf("\n\033[K> ");
                col = PROMPT_COLS;
            }
            putchar(buf[sp_start + k]);
            col++;
        }
    }

    fflush(stdout);
}

static int erase_word(char *buf, int len) {
    while (len > 0 && isspace((unsigned char)buf[len - 1]))
        len--;
    while (len > 0 && !isspace((unsigned char)buf[len - 1]))
        len--;
    return len;
}

int main(void) {
    char buf[BUFSIZE];
    int  len = 0;
    char c;

    unsigned char sys_erase, sys_kill;

    setvbuf(stdout, NULL, _IONBF, 0);

    enable_raw_mode();

    sys_erase = orig_termios.c_cc[VERASE];
    sys_kill  = orig_termios.c_cc[VKILL];

    printf("> ");

    for (;;) {
        if (read(STDIN_FILENO, &c, 1) != 1)
            continue;

        unsigned char uc = (unsigned char)c;

        if (uc == 4) {
            if (len == 0) {
                printf("\nExiting...\n");
                break;
            } else {
                continue;
            }
        }

        if (uc == sys_erase || uc == 0x7F || uc == 8) {
            if (len > 0) {
                len--;
                redraw(buf, len);
            }
            continue;
        }

        if (uc == sys_kill || uc == 0x15) {
            len = 0;
            redraw(buf, len);
            continue;
        }

        if (uc == 0x17) {
            len = erase_word(buf, len);
            redraw(buf, len);
            continue;
        }

        if (uc == '\n' || uc == '\r') {
            printf("\n> ");
            len = 0;
            continue;
        }

        if (isprint(uc)) {
            if (len < BUFSIZE - 1) {
                buf[len++] = (char)uc;
                redraw(buf, len);
            }
            continue;
        }

        printf("\a");
    }

    disable_raw_mode();
    return 0;
}