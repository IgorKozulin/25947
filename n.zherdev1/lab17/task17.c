#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_LINE 40
#define CTRL_D 4
#define CTRL_G 7
#define CTRL_W 23

// Стирает один символ с экрана
void erase_char(void)
{
    printf("\b \b");
    fflush(stdout);
}

int main(void)
{
    struct termios old_term, new_term;
    char line[MAX_LINE + 1];
    int length = 0;
    unsigned char c;

    if (tcgetattr(STDIN_FILENO, &old_term) == -1) {
        perror("tcgetattr");
        return 1;
    }

    new_term = old_term;

    // Отключаем эхо и канонический режим
    new_term.c_lflag &= ~(ECHO | ICANON);
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_term) == -1) {
        perror("tcsetattr");
        return 1;
    }

    while (read(STDIN_FILENO, &c, 1) == 1) {

        // CTRL-D в начале строки завершает программу
        if (c == CTRL_D && length == 0)
            break;

        // Enter начинает новую строку
        if (c == '\n' || c == '\r') {
            printf("\n");
            fflush(stdout);
            length = 0;
            continue;
        }

        // ERASE стирает последний символ
        if (c == old_term.c_cc[VERASE]) {
            if (length > 0) {
                length--;
                erase_char();
            }
            continue;
        }

        // KILL стирает всю текущую строку
        if (c == old_term.c_cc[VKILL]) {
            while (length > 0) {
                length--;
                erase_char();
            }
            continue;
        }

        // CTRL-W стирает последнее слово и пробелы после него
        if (c == CTRL_W) {
            while (length > 0 && line[length - 1] == ' ') {
                length--;
                erase_char();
            }

            while (length > 0 && line[length - 1] != ' ') {
                length--;
                erase_char();
            }

            continue;
        }

        // Остальные непечатаемые символы вызывают звуковой сигнал
        if (!isprint(c)) {
            putchar(CTRL_G);
            fflush(stdout);
            continue;
        }

        // Если слово пересекает 40-й столбец, переносим его целиком
        if (length == MAX_LINE && c != ' ') {
            int start = length;

            while (start > 0 && line[start - 1] != ' ')
                start--;

            if (start > 0) {
                int word_length = length - start;

                for (int i = 0; i < word_length; i++)
                    erase_char();

                printf("\n");

                memmove(line, line + start, word_length);
                length = word_length;

                printf("%.*s", length, line);
            } else {
                printf("\n");
                length = 0;
            }
        }

        // Если строка уже полностью заполнена, начинаем новую
        if (length == MAX_LINE) {
            printf("\n");
            length = 0;
        }

        line[length++] = c;

        putchar(c);
        fflush(stdout);
    }

    // Возвращаем исходные параметры терминала
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);

    printf("\n");

    return 0;
}
