#include <ctype.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int fd[2], c, status;
    unsigned char byte;
    ssize_t n;
    pid_t pid;

    if (pipe(fd) == -1 || (pid = fork()) == -1) {
        perror("pipe or fork");
        return 1;
    }

    if (pid == 0) {
        size_t count = 0;
        close(fd[1]);

        while ((n = read(fd[0], &byte, 1)) == 1) {
            if (putchar(toupper(byte)) == EOF)
                return 1;
            if (byte != '\n')
                count++;
        }

        close(fd[0]);
        if (n < 0) {
            perror("read");
            return 1;
        }

        printf("\nCharacters: %zu\n", count);
        return fflush(stdout) == EOF;
    }

    close(fd[0]);
    while ((c = getchar()) != EOF) {
        byte = (unsigned char)c;
        if (write(fd[1], &byte, 1) != 1) {
            perror("write");
            break;
        }
    }
    close(fd[1]);

    if (waitpid(pid, &status, 0) == -1)
        return 1;

    return c != EOF || ferror(stdin) ||
           !WIFEXITED(status) || WEXITSTATUS(status) != 0;
}
