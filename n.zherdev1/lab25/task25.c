#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int fd[2];

    // Создаём программный канал.
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        char c;

        // Дочерний процесс только читает из канала.
        close(fd[1]);

        while (read(fd[0], &c, 1) > 0) {
            c = toupper((unsigned char)c);
            write(STDOUT_FILENO, &c, 1);
        }

        close(fd[0]);
        _exit(0);
    }

    // Родительский процесс только пишет в канал.
    close(fd[0]);

    const char text[] = "Hello, Unix Pipe!\n";

    write(fd[1], text, sizeof(text) - 1);

    close(fd[1]);

    // Ждём завершения дочернего процесса.
    waitpid(pid, NULL, 0);

    return 0;
}
