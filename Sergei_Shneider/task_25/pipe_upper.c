#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int pipefd[2];
    pid_t pid;
    char buffer[4096];
    int error = 0;

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
        ssize_t count;
        close(pipefd[1]);

        while (1) {
            count = read(pipefd[0], buffer, sizeof(buffer));
            if (count == -1 && errno == EINTR) {
                continue;
            }
            if (count <= 0) {
                if (count == -1) {
                    perror("read");
                    error = 1;
                }
                break;
            }

            for (ssize_t i = 0; i < count; i++) {
                buffer[i] = toupper((unsigned char)buffer[i]);
            }

            if (fwrite(buffer, 1, count, stdout) != (size_t)count ||
                fflush(stdout) == EOF) {
                perror("stdout");
                error = 1;
                break;
            }
        }

        close(pipefd[0]);
        return error;
    }

    close(pipefd[0]);

    int c;
    int line_start = 1;

    while ((c = getchar()) != EOF) {
        unsigned char byte = (unsigned char)c;
        ssize_t count;

        if (c == '\n' && line_start) {
            break;
        }

        do {
            count = write(pipefd[1], &byte, 1);
        } while (count == -1 && errno == EINTR);

        if (count == -1) {
            perror("write");
            error = 1;
            break;
        }

        line_start = (c == '\n');
    }

    if (ferror(stdin)) {
        perror("stdin");
        error = 1;
    }

    close(pipefd[1]);

    int status;
    pid_t result;

    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("waitpid");
        return 1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        error = 1;
    }

    return error;
}
