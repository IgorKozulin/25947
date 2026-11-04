#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#define MAX_LINE 40      /* Ограничение длины строки в столбцах (п.7) */
#define MAX_BUF  4096    /* Размер буфера ввода */

/* Сохранённые исходные настройки терминала — нужны для восстановления */
static struct termios orig_termios;
/* Системные символы ERASE и KILL, считанные из termios */
static char erase_char;
static char kill_char;

/* Восстановление терминала в исходное состояние */
static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/* Перевод терминала в raw-режим: отключаем эхо и каноническую обработку */
static void enable_raw_mode(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);

    /* Считываем системные ERASE и KILL до изменения настроек */
    erase_char = orig_termios.c_cc[VERASE];
    kill_char  = orig_termios.c_cc[VKILL];

    raw = orig_termios;
    /* ECHO — выключаем эхо (п.1: символы показываем сами)
       ICANON — выключаем канонический режим (построчную обработку)
       IEXTEN — выключаем расширенную обработку (чтобы Ctrl+W и др.
                не обрабатывались ядром, а доходили до нас) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    /* VMIN=1 — read() возвращается после 1 байта; VTIME=0 — без таймаута */
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/* Буфер текущей строки и позиция конца ввода */
static char buffer[MAX_BUF];
static int  pos = 0;

/*
 * Полная перерисовка строки с учётом переноса слов (п.7).
 * Идём по буферу, разбивая его на слова и хвостовые пробелы.
 * Если слово не помещается в остаток строки (col + len > MAX_LINE),
 * переносим его целиком на новую строку.
 * Если слово само длиннее MAX_LINE — принудительно рвём его.
 */
static void redraw(void) {
    /* Возврат каретки в начало строки и очистка до конца */
    printf("\r\033[K");

    int col = 0;   /* текущий столбец на экране */
    int i = 0;

    while (i < pos) {
        /* Обработка пробелов */
        if (buffer[i] == ' ') {
            if (col < MAX_LINE) {
                putchar(' ');
                col++;
            } else {
                printf("\n\033[K");
                col = 0;
            }
            i++;
            continue;
        }

        /* Находим границы слова */
        int word_start = i;
        while (i < pos && buffer[i] != ' ')
            i++;
        int word_len = i - word_start;

        /* Вывод слова с учётом возможного переноса */
        if (word_len > MAX_LINE) {
            /* Слово слишком длинное — рвём на куски по MAX_LINE */
            int written = 0;
            while (written < word_len) {
                if (col == MAX_LINE) {
                    printf("\n\033[K");
                    col = 0;
                }
                int chunk = MAX_LINE - col;
                if (chunk > word_len - written)
                    chunk = word_len - written;
                
                fwrite(&buffer[word_start + written], 1, chunk, stdout);
                col += chunk;
                written += chunk;

                if (written < word_len) {
                    printf("\n\033[K");
                    col = 0;
                }
            }
        } else {
            /* Обычное слово: проверяем, поместится ли оно в остаток строки */
            if (col > 0 && col + word_len > MAX_LINE) {
                printf("\n\033[K");
                col = 0;
            }
            fwrite(&buffer[word_start], 1, word_len, stdout);
            col += word_len;
        }

        /* Обработка хвостовых пробелов (если есть) */
        int tail_start = i;
        while (i < pos && buffer[i] == ' ')
            i++;
        int tail_len = i - tail_start;

        for (int k = 0; k < tail_len; k++) {
            if (col < MAX_LINE) {
                putchar(' ');
                col++;
            } else {
                printf("\n\033[K");
                col = 0;
            }
        }
    }
    fflush(stdout);
}

int main(void) {
    /* При любом выходе восстанавливаем терминал */
    atexit(disable_raw_mode);
    enable_raw_mode();

    /* Приглашение в начале работы */
    printf("> ");
    fflush(stdout);

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        /* CTRL-D (код 4): выход, только если строка пуста (п.5) */
        if (c == 4) {
            if (pos == 0)
                break;
            continue;
        }

        /* ERASE: стираем последний символ строки (п.2) */
        if (c == erase_char) {
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                redraw();
            }
            continue;
        }

        /* KILL: стираем всю строку (п.3) */
        if (c == kill_char) {
            pos = 0;
            buffer[0] = '\0';
            redraw();
            continue;
        }

        /* CTRL-W (код 23): удаляем последнее слово (п.4) */
        if (c == 23) {
            int i = pos;
            while (i > 0 && buffer[i - 1] == ' ')
                i--;
            while (i > 0 && buffer[i - 1] != ' ')
                i--;
            pos = i;
            buffer[pos] = '\0';
            redraw();
            continue;
        }

        /* Enter: выводим строку и начинаем новую */
        if (c == '\n' || c == '\r') {
            putchar('\n');
            pos = 0;
            buffer[0] = '\0';
            printf("> ");
            fflush(stdout);
            continue;
        }

        /* Печатаемый символ: добавляем в буфер и перерисовываем (п.1) */
        if (isprint((unsigned char)c)) {
            if (pos < MAX_BUF - 1) {
                buffer[pos++] = c;
                buffer[pos] = '\0';
                redraw();
            }
            continue;
        }

        /* Прочие непечатаемые символы — звуковой сигнал CTRL-G (п.6) */
        putchar('\a');
        fflush(stdout);
    }

    /* Восстановление терминала и завершение (п.5) */
    disable_raw_mode();
    printf("\nExiting...\n");
    return 0;
}
