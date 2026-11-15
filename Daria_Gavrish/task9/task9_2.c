
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execlp("cat", "cat", "long.txt", (char *)NULL);
        perror("execlp");
        _exit(1);
    }

    printf("Parent: waiting for child process.\n");
    fflush(stdout);

    if (waitpid(pid, NULL, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    printf("Parent: child process finished.\n");

    return 0;
}

