#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Использование: %s команда [аргументы...]\n", argv[0]);
        return 1;
    }

    pid_t pid = fork(); // Создаём потомка
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        execvp(argv[1], &argv[1]);  // заменяем образ текущего процесса образом новой программы,
        perror(argv[1]);
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) { // если состояние дочернего процесса изменилось
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) // проверяем, что процесс завершился нормально
        printf("Код завершения: %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status)) // если нет, то проверяем, был ли процесс убит сигналом.
        printf("Процесс убит сигналом %d\n", WTERMSIG(status));
    return 0;
}