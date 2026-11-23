
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "shell.h"

int promptline(char *prompt, char *line, int sizline){
    int n = 0;
    ssize_t count;

    write(STDOUT_FILENO, prompt, strlen(prompt));

    while (n < sizline - 1){
        count = read(STDIN_FILENO, line + n, 1);

        if (count <= 0){
            break;
        }

        n += count;

        if (line[n - 1] == '\n'){
            if (n >= 2 && line[n - 2] == '\\'){
                line[n - 2] = ' ';
                line[n - 1] = ' ';
                continue;
            }

            break;
        }
    }

    line[n] = '\0';

    return n;
}

