#define _POSIX_C_SOURCE 200112L
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#define WIDTH 40

static void erase_char(void)
{
    printf("\b \b");
}

int main(void)
{
    struct termios saved, settings;

    if (tcgetattr(STDIN_FILENO, &saved) == -1) {
        perror("tcgetattr");
        return 1;
    }

    settings = saved;
    settings.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    settings.c_iflag &= ~(IXON | ICRNL | INLCR | IGNCR);
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &settings) == -1) {
        perror("tcsetattr");
        return 1;
    }

    char line[WIDTH];
    int length = 0;
    int status = 0;
    unsigned char c;

    for (;;) {
        ssize_t result = read(STDIN_FILENO, &c, 1);

        if (result == -1) {
            if (errno == EINTR)
                continue;

            perror("read");
            status = 1;
            break;
        }

        if (result == 0)
            break;

        if (c == 4) {
            if (length == 0)
                break;

            putchar('\a');
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

                while (start > 0 && line[start - 1] != ' ')
                    start--;

                if (c == ' ') {
                    putchar('\n');
                    length = 0;
                    fflush(stdout);
                    continue;
                }

                if (start > 0) {
                    for (int i = start; i < length; i++)
                        erase_char();

                    putchar('\n');

                    int count = length - start;

                    for (int i = 0; i < count; i++) {
                        line[i] = line[start + i];
                        putchar(line[i]);
                    }

                    length = count;
                } else {
                    putchar('\n');
                    length = 0;
                }
            }

            line[length++] = (char)c;
            putchar(c);
        } else {
            putchar('\a');
        }

        fflush(stdout);
    }

    putchar('\n');
    fflush(stdout);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved) == -1) {
        perror("tcsetattr");
        status = 1;
    }

    return status;
}
