#define _POSIX_C_SOURCE 200112L
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int no_wait = argc == 3 && strcmp(argv[1], "--no-wait") == 0;

    if (argc != 2 && !no_wait) {
        fprintf(stderr, "Usage: %s [--no-wait] filename\n", argv[0]);
        return 1;
    }

    const char *filename = argv[no_wait ? 2 : 1];
    pid_t child = fork();

    if (child == -1) {
        perror("fork");
        return 1;
    }

    if (child == 0) {
        execlp("cat", "cat", filename, (char *)NULL);
        perror("execlp cat");
        _exit(127);
    }

    printf("[PARENT] Child PID: %ld\n", (long)child);
    fflush(stdout);

    if (no_wait) {
        printf("[PARENT] Final line (without waiting)\n");
        return 0;
    }

    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno == EINTR)
            continue;

        perror("waitpid");
        return 1;
    }

    int code = 1;
    if (WIFEXITED(status)) {
        code = WEXITSTATUS(status);
        printf("[PARENT] cat exit code: %d\n", code);
    } else if (WIFSIGNALED(status)) {
        printf("[PARENT] cat terminated by signal: %d\n", WTERMSIG(status));
    }

    printf("[PARENT] Final line (after waitpid)\n");
    return code;
}
