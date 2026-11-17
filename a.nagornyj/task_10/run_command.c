#define _XOPEN_SOURCE 700

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
    pid_t child;
    pid_t waited;
    int child_status;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [argument ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    printf("[PARENT] Starting command: %s\n", argv[1]);
    if (fflush(stdout) == EOF) {
        perror("fflush");
        return EXIT_FAILURE;
    }

    child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(127);
    }

    do {
        waited = waitpid(child, &child_status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(child_status)) {
        int code = WEXITSTATUS(child_status);
        printf("[PARENT] Command exited normally with code: %d\n", code);
        return code;
    }
    if (WIFSIGNALED(child_status)) {
        int signal_number = WTERMSIG(child_status);
        const char *description = strsignal(signal_number);
        printf("[PARENT] Command terminated by signal: %d (%s)\n",
            signal_number, description != NULL ? description : "unknown");
        return 128 + signal_number;
    }
    fprintf(stderr, "[PARENT] Child ended in an unexpected state.\n");
    return EXIT_FAILURE;
}
