#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    pid_t child = fork();

    if (child == -1) {
        perror("fork");
        return 1;
    }

    if (child == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(127);
    }

    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno == EINTR)
            continue;

        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("Exit code: %d\n", code);
        return code;
    }

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("Terminated by signal: %d (%s)\n", sig, strsignal(sig));
        return 128 + sig;
    }

    return 1;
}
