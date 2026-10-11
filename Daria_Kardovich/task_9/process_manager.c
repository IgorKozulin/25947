#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("[CHILD] Printing file:\n");
        fflush(stdout);
        execlp("cat", "cat", argv[1], NULL);
        perror("execlp");
        return 1;
    }

    printf("[PARENT] Child PID: %d\n", pid);
    printf("[PARENT] I am working while child prints the file.\n");

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("[PARENT] Child exit code: %d\n", WEXITSTATUS(status));
    }

    printf("[PARENT] Child has finished. This is the last parent line.\n");

    return 0;
}
