#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    pid_t pid;
    int status;

    /* 1. Проверка аргументов */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    printf("[PARENT] Starting command: %s\n", argv[1]);

    /* 2. Создание подпроцесса */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* 3. Дочерний процесс */
        /* argv[0] - имя нашей программы, argv[1] - команда, поэтому передаем &argv[1] */
        execvp(argv[1], &argv[1]);
        
        /* Сюда попадаем, только если execvp не сработал (команда не найдена) */
        perror("Execvp failed");
        exit(127);
    } else {
        /* 4. Родительский процесс */
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }

        /* 5. Анализ статуса завершения */
        if (WIFEXITED(status)) {
            printf("[PARENT] Command exited normally with code: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            printf("[PARENT] Command terminated by signal: %d (%s)\n", sig, strsignal(sig));
        }
    }

    return 0;
}
