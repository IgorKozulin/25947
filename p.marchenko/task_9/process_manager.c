#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    pid_t pid;
    int status;

    /* 1. Проверка аргументов */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    printf("[PARENT] Starting program. My PID: %d\n", getpid());

    /* 2. Создание подпроцесса */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* 3. Дочерний процесс */
        printf("[CHILD] I am the child process. My PID: %d\n", getpid());
        execlp("cat", "cat", argv[1], (char *)NULL);
        /* Сюда попадаем, только если execlp не сработал */
        perror("execlp");
        exit(1);
    } else {
        /* 4. Родительский процесс */
        printf("[PARENT] I am the parent. Child PID: %d\n", pid);
        printf("[PARENT] Printing some text while child is working...\n");
        fflush(stdout);

        /* Ждем завершения ребенка */
        printf("[PARENT] Waiting for child to finish...\n");
        fflush(stdout);

        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status)) {
            printf("[PARENT] Child exited with status: %d\n", WEXITSTATUS(status));
        } else {
            printf("[PARENT] Child terminated abnormally\n");
        }

        /* 5. Финальная строка родителя */
        printf("[PARENT] Child has finished. This is the last line printed by parent.\n");
    }

    return 0;
}
