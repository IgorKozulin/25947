#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execlp("cat", "cat", argv[1], (char *)NULL);
        perror("execlp");
        _exit(127);
    }

    printf("Parent is waiting\n");
    fflush(stdout);

    while (waitpid(pid, NULL, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }

    printf("\nChild has finished\n");
    return 0;
}
