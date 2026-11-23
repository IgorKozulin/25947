#define _POSIX_C_SOURCE 200112L
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int write_all(int fd, const char *text, size_t length)
{
    while (length > 0) {
        ssize_t n = write(fd, text, length);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        text += n;
        length -= (size_t)n;
    }
    return 0;
}

static int print_upper(int fd)
{
    char buffer[1024];

    for (;;) {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n == 0)
            return 0;

        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            return 1;
        }

        for (ssize_t i = 0; i < n; i++)
            buffer[i] = (char)toupper((unsigned char)buffer[i]);

        if (printf("%.*s", (int)n, buffer) < 0 || fflush(stdout) == EOF) {
            perror("stdout");
            return 1;
        }
    }
}

int main(void)
{
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (pid == 0) {
        close(pipefd[1]);
        int result = print_upper(pipefd[0]);
        close(pipefd[0]);
        return result;
    }

    close(pipefd[0]);

    if (isatty(STDIN_FILENO))
        fputs("Enter text (empty line or Ctrl+D to finish):\n", stderr);

    char line[1024];
    int at_line_start = 1;
    int result = 0;

    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (at_line_start && line[0] == '\n')
            break;

        size_t length = strlen(line);
        if (write_all(pipefd[1], line, length) == -1) {
            perror("write");
            result = 1;
            break;
        }

        at_line_start = length > 0 && line[length - 1] == '\n';
    }

    if (ferror(stdin)) {
        perror("fgets");
        result = 1;
    } else if (result == 0 && !at_line_start) {
        if (write_all(pipefd[1], "\n", 1) == -1) {
            perror("write");
            result = 1;
        }
    }

    close(pipefd[1]);

    int status;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        perror("waitpid");
        return 1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return 1;

    return result;
}
