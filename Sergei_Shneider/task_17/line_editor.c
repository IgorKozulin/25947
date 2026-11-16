#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>

#define WIDTH 40

static void erase_char(void)
{
    fputs("\b \b", stdout);
}

int main(void)
{
    struct termios saved, settings;
    char line[WIDTH];
    unsigned char c;
    int length = 0;
    int error = 0;
    ssize_t result;

    if (tcgetattr(STDIN_FILENO, &saved) == -1) {
        perror("tcgetattr");
        return 1;
    }

    settings = saved;
    settings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    settings.c_iflag &= ~IXON;
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) == -1) {
        perror("tcsetattr");
        return 1;
    }

    while (1) {
        result = read(STDIN_FILENO, &c, 1);

        if (result == -1 && errno == EINTR) {
            continue;
        }

        if (result <= 0) {
            if (result == -1) {
                perror("read");
                error = 1;
            }
            break;
        }

        if (c == 4) {
            if (length == 0) {
                break;
            }
        } else if (c == saved.c_cc[VERASE]) {
            if (length > 0) {
                length--;
                erase_char();
            }
        } else if (c == saved.c_cc[VKILL]) {
            while (length > 0) {
                length--;
                erase_char();
            }
        } else if (c == 23) {
            while (length > 0 && line[length - 1] == ' ') {
                length--;
                erase_char();
            }
            while (length > 0 && line[length - 1] != ' ') {
                length--;
                erase_char();
            }
        } else if (c == '\n' || c == '\r') {
            putchar('\n');
            length = 0;
        } else if (isprint(c)) {
            if (length == WIDTH) {
                int start = length;

                if (c != ' ') {
                    while (start > 0 && line[start - 1] != ' ') {
                        start--;
                    }
                }

                if (start == 0) {
                    start = length;
                }

                for (int i = start; i < length; i++) {
                    erase_char();
                }

                putchar('\n');
                length -= start;
                memmove(line, line + start, length);
                fwrite(line, 1, length, stdout);
            }

            line[length++] = c;
            putchar(c);
        } else {
            putchar('\a');
        }

        fflush(stdout);
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &saved) == -1) {
        perror("tcsetattr");
        return 1;
    }

    return error;
}
