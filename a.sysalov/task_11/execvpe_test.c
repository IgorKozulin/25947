#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

extern char **environ;

int execvpe(const char *file, char *const argv[], char *const envp[])
{
    char **saved_environment = environ;
    environ = (char **)envp;
    execvp(file, argv);
    int saved_errno = errno;
    environ = saved_environment;
    errno = saved_errno;
    return -1;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [arguments...]\n", argv[0]);
        return 1;
    }
    char *environment[] = {"PATH=/usr/bin:/bin", "TASK=11", NULL};
    execvpe(argv[1], argv + 1, environment);
    perror("execvpe");
    return 127;
}
