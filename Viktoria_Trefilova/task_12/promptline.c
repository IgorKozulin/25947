#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"

int promptline(const char *prompt, char *line, int sizline)
{
    int length = 0;
    int too_long = 0;
    char previous = '\0';
    char ch;
    ssize_t result;

    if (write(STDOUT_FILENO, prompt, strlen(prompt)) == -1) {
        perror("write");
        return -1;
    }

    /* Read through one logical line without taking input from a child. */
    for (;;) {
        result = read(STDIN_FILENO, &ch, 1);
        if (result == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            return -1;
        }
        if (result == 0)
            break;

        if (ch == '\n') {
            if (previous == '\\') {
                if (!too_long)
                    line[length - 1] = ' ';
                previous = '\0';
                continue;
            }

            if (length < sizline - 1)
                line[length++] = ch;
            break;
        }

        if (length < sizline - 1)
            line[length++] = ch;
        else
            too_long = 1;
        previous = ch;
    }

    if (too_long) {
        fprintf(stderr, "command line too long\n");
        line[0] = '\0';
        return 1;
    }

    line[length] = '\0';
    return length;
}
