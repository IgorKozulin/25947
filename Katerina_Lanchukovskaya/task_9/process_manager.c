#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv){
    pid_t pid = fork();

    if (pid == 0){
        execlp("cat", "cat", argv[1], NULL);

        perror("execlp");
        exit(1);
    }
    else if (pid > 0){
        printf("Parent process started\n");
        printf("Parent is working\n");

        waitpid(pid, NULL,0);
        printf("Child finished. Last parent message\n");
    }
    else{
        perror("fork");
        return 1;
    }

    return 0;
}
