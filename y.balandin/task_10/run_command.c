#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    printf("[PARENT] Starting command: %s\n", argv[1]);

    pid_t pid = fork(); // клонируем
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* --- Ребёнок: заменяем себя на команду --- */
        execvp(argv[1], &argv[1]);

        /* Сюда попадаем только если execvp не сработал */
        perror("Execvp failed");
        exit(127);   /* 127 = command not found */
    }

    /* --- Родитель: ждём и разбираем статус --- */
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("[PARENT] Command exited normally with code: %d\n",
               WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("[PARENT] Command terminated by signal: %d (%s)\n",
               sig, strsignal(sig));
    } else {
        printf("[PARENT] Command terminated abnormally\n");
    }

    return 0;
}
