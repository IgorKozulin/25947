#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#define BUF_SIZE 1024

int main(void)
{
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(pipefd[1]);

        char buf[BUF_SIZE];
        ssize_t n;

        while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            for (ssize_t i = 0; i < n; i++)
                buf[i] = (char)toupper((unsigned char)buf[i]);
            printf("%s", buf);
            fflush(stdout);
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    }

    close(pipefd[0]);

    char line[BUF_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL)
            break;

        if (line[0] == '\n' || line[0] == '\0')
            break;

        if (write(pipefd[1], line, strlen(line)) == -1) {
            perror("write");
            break;
        }
    }

    close(pipefd[1]);
    wait(NULL);
    printf("Done.\n");

    return 0;
}
