#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("[PARENT] Starting program. My PID: %d\n", (int)getpid());

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        printf("[CHILD] I am the child process. My PID: %d\n", (int)getpid());
        execlp("cat", "cat", argv[1], NULL);

        perror("execlp");
        exit(EXIT_FAILURE);
    }

    printf("[PARENT] I am the parent. Child PID: %d\n", (int)pid);
    printf("[PARENT] Printing some text while child is working...\n");
    fflush(stdout);

    printf("[PARENT] Waiting for child to finish...\n");
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    if (WIFEXITED(status)) {
        printf("[PARENT] Child exited with status: %d\n",
               WEXITSTATUS(status));
    } else {
        printf("[PARENT] Child terminated abnormally\n");
    }

    printf("[PARENT] Child has finished. "
           "This is the last line printed by parent.\n");

    return EXIT_SUCCESS;
}