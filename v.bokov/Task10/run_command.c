
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Enter command\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        execvp(argv[1], &argv[1]);

        perror("execvp");
        _exit(127);
    }
    else
    {
        int status;

        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status))
        {
            printf("Exit code: %d\n",
                   WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Terminated by signal: %d\n",
                   WTERMSIG(status));
        }
    }

    return 0;
}
