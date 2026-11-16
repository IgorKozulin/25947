#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    // Создаём дочерний процесс
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Запускаем команду и передаём ей все оставшиеся аргументы
        execvp(argv[1], &argv[1]);

        // Выполнится только при ошибке execvp()
        perror("execvp");
        _exit(1);
    }

    int status;

    // Ждём завершения дочернего процесса
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    // Выводим обычный код завершения команды
    if (WIFEXITED(status))
        printf("Exit code: %d\n", WEXITSTATUS(status));
    else
        printf("Process did not exit normally\n");

    return 0;
}
