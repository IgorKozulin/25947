#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define WIDTH 40

static struct termios original_termios;
static int terminal_changed = 0;

static void restore_terminal(void)
{
    if (terminal_changed) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    }
}

static void print_text(const char *text, size_t size)
{
    write(STDOUT_FILENO, text, size);
}

static void erase_last(char *line, size_t *length)
{
    if (*length > 0) {
        print_text("\b \b", 3);
        (*length)--;
        line[*length] = '\0';
    }
}

static size_t find_word_start(const char *line, size_t length)
{
    while (length > 0 && !isspace((unsigned char)line[length - 1])) {
        length--;
    }

    return length;
}

int main(void)
{
    struct termios raw_termios;
    char line[WIDTH + 1];
    size_t length = 0;
    unsigned char symbol;
    unsigned char erase_key;
    unsigned char kill_key;

    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        return 1;
    }

    raw_termios = original_termios;
    raw_termios.c_lflag &= ~(ICANON | ECHO);
    raw_termios.c_iflag &= ~(ICRNL | IXON);
    raw_termios.c_cc[VMIN] = 1;
    raw_termios.c_cc[VTIME] = 0;

    erase_key = original_termios.c_cc[VERASE];
    kill_key = original_termios.c_cc[VKILL];

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios) == -1) {
        perror("tcsetattr");
        return 1;
    }

    terminal_changed = 1;
    atexit(restore_terminal);

    print_text("Line editor. Ctrl-D on an empty line exits.\r\n", 43);

    while (read(STDIN_FILENO, &symbol, 1) == 1) {
        if (symbol == 4) {
            if (length == 0) {
                print_text("\r\n", 2);
                break;
            }

            print_text("\a", 1);
            continue;
        }

        if (symbol == '\r' || symbol == '\n') {
            print_text("\r\n", 2);
            length = 0;
            line[0] = '\0';
            continue;
        }

        if (symbol == erase_key || symbol == 127 || symbol == 8) {
            erase_last(line, &length);
            continue;
        }

        if (symbol == kill_key || symbol == 21) {
            while (length > 0) {
                erase_last(line, &length);
            }

            continue;
        }

        if (symbol == 23) {
            while (length > 0 && isspace((unsigned char)line[length - 1])) {
                erase_last(line, &length);
            }

            while (length > 0 && !isspace((unsigned char)line[length - 1])) {
                erase_last(line, &length);
            }

            continue;
        }

        if (!isprint(symbol) && symbol != ' ') {
            print_text("\a", 1);
            continue;
        }

        if (length == WIDTH) {
            size_t word_start = find_word_start(line, length);

            if (word_start > 0) {
                size_t word_length = length - word_start;
                char word[WIDTH + 1];

                for (size_t i = 0; i < word_length; i++) {
                    word[i] = line[word_start + i];
                }

                word[word_length] = '\0';

                while (length > word_start) {
                    erase_last(line, &length);
                }

                print_text("\r\n", 2);

                for (size_t i = 0; i < word_length; i++) {
                    line[i] = word[i];
                }

                length = word_length;
                line[length] = '\0';
                print_text(line, length);
            } else {
                print_text("\r\n", 2);
                length = 0;
                line[0] = '\0';
            }
        }

        line[length] = (char)symbol;
        length++;
        line[length] = '\0';
        write(STDOUT_FILENO, &symbol, 1);
    }

    return 0;
}
