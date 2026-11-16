#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
    pid_t child;
    int child_status;
    pid_t waited;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s text-file\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("[PARENT] Starting program. PID: %ld\n", (long)getpid());
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
        char *safe_path = argv[1];

        if (argv[1][0] == '-') {
            safe_path = malloc(strlen(argv[1]) + 3);
            if (safe_path == NULL) {
                perror("malloc");
                _exit(127);
            }
            (void)strcpy(safe_path, "./");
            (void)strcat(safe_path, argv[1]);
        }
        printf("[CHILD] Executing cat for %s\n", argv[1]);
        (void)fflush(stdout);
        execlp("cat", "cat", safe_path, (char *)NULL);
        perror("execlp(cat)");
        if (safe_path != argv[1]) {
            free(safe_path);
        }
        _exit(127);
    }

    printf("[PARENT] Child PID: %ld\n", (long)child);
    printf("[PARENT] Printing text while the child runs.\n");
    printf("[PARENT] Waiting for child to finish.\n");
    if (fflush(stdout) == EOF) {
        perror("fflush");
        return EXIT_FAILURE;
    }

    do {
        waited = waitpid(child, &child_status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(child_status)) {
        printf("[PARENT] Child exited with status: %d\n",
            WEXITSTATUS(child_status));
    } else if (WIFSIGNALED(child_status)) {
        printf("[PARENT] Child was terminated by signal: %d\n",
            WTERMSIG(child_status));
    }
    printf("[PARENT] Child has finished. This is the last line printed by parent.\n");
    return EXIT_SUCCESS;
}
