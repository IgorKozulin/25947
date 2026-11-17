#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#define MAX_LEN 4096
#define LINE_WIDTH 40

static struct termios orig_termios;
static char buffer[MAX_LEN];
static int pos = 0;
static unsigned char erase_char;
static unsigned char kill_char;

/* Восстановление терминала */
static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/* Перевод в raw-режим */
static void enable_raw_mode(void) {
    struct termios raw;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }
    atexit(disable_raw_mode);

    erase_char = orig_termios.c_cc[VERASE];
    kill_char  = orig_termios.c_cc[VKILL];

    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}

/* Перерисовка строки с учетом переносов */
static void redraw(void) {
    printf("\r\033[K"); /* Возврат каретки и очистка строки */
    
    int col = 0;
    int i = 0;
    int word_start;

    while (i < pos) {
        /* Пропускаем пробелы (они выводятся, но не считаются словом) */
        if (buffer[i] == ' ') {
            if (col >= LINE_WIDTH) {
                printf("\n\033[K");
                col = 0;
            }
            putchar(' ');
            col++;
            i++;
            continue;
        }

        /* Начало нового слова */
        word_start = i;
        int word_len = 0;
        while (i < pos && buffer[i] != ' ') {
            word_len++;
            i++;
        }

        /* Если слово не влезает в остаток строки и это не начало строки — переносим */
        if (col > 0 && col + word_len > LINE_WIDTH) {
            printf("\n\033[K");
            col = 0;
        }

        /* Выводим слово */
        for (int j = 0; j < word_len; j++) {
            putchar(buffer[word_start + j]);
            col++;
        }
    }
    fflush(stdout);
}

int main(void) {
    enable_raw_mode();
    printf("> ");
    fflush(stdout);

    char c;
    while (1) {
        if (read(STDIN_FILENO, &c, 1) != 1) break;

        if (c == 4) { /* Ctrl+D */
            if (pos == 0) {
                printf("\nExiting...\n");
                break;
            } else {
                printf("\a");
            }
        } else if (c == 10 || c == 13) { /* Enter */
            printf("\n");
            pos = 0;
            printf("> ");
            fflush(stdout);
        } else if ((unsigned char)c == erase_char || c == 127 || c == 8) { /* Backspace */
            if (pos > 0) {
                pos--;
                redraw();
            }
        } else if ((unsigned char)c == kill_char) { /* Ctrl+U (KILL) */
            pos = 0;
            redraw();
        } else if (c == 23) { /* Ctrl+W (стирание слова) */
            /* Удаляем пробелы в конце */
            while (pos > 0 && buffer[pos - 1] == ' ') pos--;
            /* Удаляем само слово */
            while (pos > 0 && buffer[pos - 1] != ' ') pos--;
            redraw();
        } else if (isprint((unsigned char)c)) {
            if (pos < MAX_LEN - 1) {
                buffer[pos++] = c;
                redraw();
            } else {
                printf("\a"); /* Достигнут лимит буфера */
            }
        } else {
            /* Все остальные непечатаемые символы — звуковой сигнал */
            printf("\a");
            fflush(stdout);
        }
    }

    return 0;
}
