#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int status;
    pid_t pid, result;

    if (argc != 2 &&
        (argc != 3 || strcmp(argv[2], "--no-wait") != 0)) {
        fprintf(stderr, "Usage: %s file [--no-wait]\n", argv[0]);
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execlp("cat", "cat", argv[1], (char *)NULL);
        perror("execlp");
        _exit(127);
    }

    printf("Parent: child started\n");
    fflush(stdout);

    if (argc == 3) {
        printf("Parent: finished\n");
        return 0;
    }

    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("waitpid");
        return 1;
    }

    printf("Parent: finished\n");
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
