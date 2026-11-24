#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        printf("PID: %d\n", getpid());
        _exit(127);
    }
    printf("Parent process started.\n");
    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }
    printf("PID: %d\n", getpid());
    printf("Child process finished.\n");
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
