#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char **argv){

    if (argc < 2){
        printf("Usage: %s command [arguments]\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid == 0){
        execvp(argv[1], &argv[1]);

        perror("execvp");
        exit(127);
    }
    else if (pid > 0){
        int status;

        if (waitpid(pid, &status, 0) == -1){
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status)){
            printf("Exit code: %d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)){
            printf("Signal: %d\n", WTERMSIG(status));
        }
    }
    else{
        perror("fork");
        return 1;
    }

    return 0;
}
