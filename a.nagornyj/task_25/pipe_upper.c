#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int
write_all(int fd, const char *buffer, size_t length)
{
    size_t written = 0;

    while (written < length) {
        ssize_t amount = write(fd, buffer + written, length - written);
        if (amount == -1 && errno == EINTR) {
            continue;
        }
        if (amount <= 0) {
            return -1;
        }
        written += (size_t)amount;
    }
    return 0;
}

static int
run_reader(int pipe_read_end, int pipe_write_end)
{
    char buffer[4096];

    if (close(pipe_write_end) == -1) {
        perror("child close(pipe write end)");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[CHILD] Waiting for text from the pipe.\n");
    for (;;) {
        ssize_t amount = read(pipe_read_end, buffer, sizeof(buffer));
        if (amount == -1 && errno == EINTR) {
            continue;
        }
        if (amount == -1) {
            perror("child read");
            (void)close(pipe_read_end);
            return EXIT_FAILURE;
        }
        if (amount == 0) {
            break;
        }
        for (ssize_t index = 0; index < amount; ++index) {
            buffer[index] = (char)toupper((unsigned char)buffer[index]);
        }
        if (fwrite(buffer, 1, (size_t)amount, stdout) != (size_t)amount ||
            fflush(stdout) == EOF) {
            perror("child output");
            (void)close(pipe_read_end);
            return EXIT_FAILURE;
        }
    }
    if (close(pipe_read_end) == -1) {
        perror("child close(pipe read end)");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int
main(void)
{
    int pipe_fd[2];
    pid_t child;
    int child_status;
    pid_t waited;
    char input[4096];
    int continuing_line = 0;
    int status = EXIT_SUCCESS;

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }
    if (fflush(NULL) == EOF) {
        perror("fflush");
        (void)close(pipe_fd[0]);
        (void)close(pipe_fd[1]);
        return EXIT_FAILURE;
    }
    child = fork();
    if (child == -1) {
        perror("fork");
        (void)close(pipe_fd[0]);
        (void)close(pipe_fd[1]);
        return EXIT_FAILURE;
    }
    if (child == 0) {
        int child_result = run_reader(pipe_fd[0], pipe_fd[1]);
        _exit(child_result);
    }

    if (close(pipe_fd[0]) == -1) {
        perror("parent close(pipe read end)");
        status = EXIT_FAILURE;
    }
    fprintf(stderr,
        "[PARENT] Enter mixed-case text; an empty line or EOF ends input.\n");
    while (status == EXIT_SUCCESS && fgets(input, sizeof(input), stdin) != NULL) {
        size_t input_length = strlen(input);
        if (!continuing_line &&
            ((input_length == 1 && input[0] == '\n') ||
             (input_length == 2 && input[0] == '\r' && input[1] == '\n'))) {
            break;
        }
        if (write_all(pipe_fd[1], input, (size_t)input_length) == -1) {
            perror("parent write to pipe");
            status = EXIT_FAILURE;
            break;
        }
        continuing_line = input_length > 0 && input[input_length - 1] != '\n';
    }
    if (ferror(stdin)) {
        perror("stdin");
        status = EXIT_FAILURE;
    }
    if (close(pipe_fd[1]) == -1) {
        perror("parent close(pipe write end)");
        status = EXIT_FAILURE;
    }

    do {
        waited = waitpid(child, &child_status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }
    if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != EXIT_SUCCESS) {
        fprintf(stderr, "[PARENT] Child did not finish successfully.\n");
        status = EXIT_FAILURE;
    } else {
        fprintf(stderr, "[PARENT] Child finished; pipe was closed cleanly.\n");
    }
    return status;
}
