#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"

int promptline(char *prompt, char *line, int sizline)
{
    int n = 0;

    if (write(STDOUT_FILENO, prompt, strlen(prompt)) == -1) {
        return -1;
    }

    while (n < sizline - 1) {
        ssize_t result = read(STDIN_FILENO, line + n, (size_t)(sizline - 1 - n));

        if (result == 0) {
            line[n] = '\0';
            return n;
        }

        if (result == -1) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        n += (int)result;
        line[n] = '\0';

        if (n >= 2 && line[n - 2] == '\\' && line[n - 1] == '\n') {
            line[n - 2] = ' ';
            line[n - 1] = ' ';
            continue;
        }

        return n;
    }

    line[sizline - 1] = '\0';
    return n;
}
