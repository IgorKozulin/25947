#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child == 0) {
        execvp(argv[1], argv + 1);
        int code = errno == ENOENT ? 127 : 126;
        perror(argv[1]);
        _exit(code);
    }

    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("Exit code: %d\n", code);
        return code;
    }

    if (WIFSIGNALED(status)) {
        int signal_number = WTERMSIG(status);
        printf("Terminated by signal: %d\n", signal_number);
        return 128 + signal_number;
    }

    return EXIT_FAILURE;
}
