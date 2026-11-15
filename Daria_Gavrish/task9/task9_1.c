#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

    printf("Parent: child process created.\n");
    printf("Parent: program finished.\n");

    return 0;
}
