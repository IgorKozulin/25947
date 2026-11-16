#define _POSIX_C_SOURCE 200809L
#include <string.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int write_all(int fd, const char *buffer, size_t length)
{
    while (length > 0) {
        ssize_t written = write(fd, buffer, length);

        if (written == -1) {
            return -1;
        }

        buffer += written;
        length -= (size_t)written;
    }

    return 0;
}

int main(void)
{
    int pipefd[2];
    pid_t pid;
    char buffer[1024];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (pid == 0) {
        close(pipefd[1]);

        while (1) {
            ssize_t count = read(pipefd[0], buffer, sizeof(buffer));

            if (count == 0) {
                break;
            }

            if (count == -1) {
                perror("read");
                close(pipefd[0]);
                return 1;
            }

            for (ssize_t i = 0; i < count; i++) {
                buffer[i] = (char)toupper((unsigned char)buffer[i]);
            }

            if (write_all(STDOUT_FILENO, buffer, (size_t)count) == -1) {
                perror("write");
                close(pipefd[0]);
                return 1;
            }
        }

        close(pipefd[0]);
        return 0;
    }

    close(pipefd[0]);

    printf("Enter text. Empty line or Ctrl+D finishes input.\n");
    fflush(stdout);

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (buffer[0] == '\n') {
            break;
        }

        if (write_all(pipefd[1], buffer, strlen(buffer)) == -1) {
            perror("write");
            close(pipefd[1]);
            waitpid(pid, NULL, 0);
            return 1;
        }
    }

    close(pipefd[1]);

    if (waitpid(pid, NULL, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    printf("Child process finished.\n");
    return 0;
}
