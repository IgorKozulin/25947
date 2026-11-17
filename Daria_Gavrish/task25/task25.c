
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return 1;
    }

    if (pid == 0) {
        close(fd[1]);

        char buffer[256];
        ssize_t count;

        while ((count = read(fd[0], buffer, sizeof(buffer))) > 0) {
            for (ssize_t i = 0; i < count; i++) {
                buffer[i] = (char)toupper((unsigned char)buffer[i]);
            }

            if (fwrite(buffer, 1, (size_t)count, stdout) != (size_t)count) {
                perror("fwrite");
                close(fd[0]);
                return 1;
            }
        }

        if (count == -1) {
            perror("read");
            close(fd[0]);
            return 1;
        }

        close(fd[0]);
        return 0;
    }

    close(fd[0]);

    const char text[] = "Hello World! This is a Pipe.\n";
    size_t length = sizeof(text) - 1;
    size_t written = 0;

    while (written < length) {
        ssize_t count = write(fd[1], text + written, length - written);

        if (count <= 0) {
            perror("write");
            close(fd[1]);
            waitpid(pid, NULL, 0);
            return 1;
        }

        written += (size_t)count;
    }

    close(fd[1]);

    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return 1;
    }

    return 0;
}

