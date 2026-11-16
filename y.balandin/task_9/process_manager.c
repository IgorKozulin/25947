#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    printf("[PARENT] Starting program. My PID: %d\n", getpid());

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* --- Дочерний процесс: запускаем cat --- */
        printf("[CHILD] I am the child. My PID: %d\n", getpid());

        execlp("cat", "cat", argv[1], NULL);

        /* Сюда попадём только если execlp не сработал */
        perror("execlp");
        _exit(EXIT_FAILURE);
    }

    /* --- Родительский процесс --- */
    printf("[PARENT] I am the parent. Child PID: %d\n", pid);
    printf("[PARENT] Printing some text while child is working...\n");

    printf("[PARENT] Waiting for child to finish...\n");
    int status;
    waitpid(pid, &status, 0);   /* ждём завершения ребёнка */

    if (WIFEXITED(status))
        printf("[PARENT] Child exited with status: %d\n", WEXITSTATUS(status));

    /* ЭТА строка — строго последняя, после завершения cat */
    printf("[PARENT] Child has finished. This is the last line printed by parent.\n");

    return 0;
}
