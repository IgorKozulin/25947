#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(){
    int pipefd[2];
    pid_t pid;
    char buffer[256];
    ssize_t n;

    if (pipe(pipefd) == -1){
        perror("pipe");
        return 1;
    }

    pid = fork();

    if (pid == -1){
        perror("fork");
        return 1;
    }

    if (pid == 0){
        close(pipefd[1]);

        while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0){
            for (ssize_t i = 0; i < n; i++){
                buffer[i] = toupper((unsigned char)buffer[i]);
            }

            write(STDOUT_FILENO, buffer, n);
        }

        close(pipefd[0]);
        _exit(0);
    }
    else {
        close(pipefd[0]);

        printf("Enter text (empty line or Ctrl+D to exit):\n");

        while (fgets(buffer, sizeof(buffer), stdin) != NULL){
            if (strcmp(buffer, "\n") == 0){
                break;
            }

            size_t len = strlen(buffer);
            size_t sent = 0;

            while (sent < len){
                ssize_t written = write(pipefd[1], buffer + sent, len - sent);

                if (written == -1){
                    perror("write");
                    close(pipefd[1]);
                    wait(NULL);
                    return 1;
                }

                sent += written;
            }
        }

        close(pipefd[1]);
        wait(NULL);
    }

    return 0;
}
