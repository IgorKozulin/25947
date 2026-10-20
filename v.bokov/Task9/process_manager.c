
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Enter filename\n");
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
        printf("Child process: PID = %ld\n",
               (long)getpid());
        fflush(stdout);

        execlp("cat", "cat", argv[1], (char *)NULL);

        perror("execlp");
        _exit(1);
    }
    else
    {
        int status;

        printf("Parent process: PID = %ld\n",
               (long)getpid());

        printf("Child PID = %ld\n", (long)pid);

        printf("Parent is working...\n");
        fflush(stdout);

        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status))
        {
            printf("Child exit status = %d\n",
                   WEXITSTATUS(status));
        }

        printf("Parent: child finished\n");
    }

    return 0;
}
