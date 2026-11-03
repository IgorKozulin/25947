#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s file\n", argv[0]);
        return 1;
    }

    // Создаём дочерний процесс
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Дочерний процесс заменяется программой cat
        execlp("cat", "cat", argv[1], NULL);

        // Выполнится только если execlp() завершился с ошибкой
        perror("execlp");
        return 1;
    }

    // Этот текст родитель может вывести одновременно с работой cat
    printf("Parent: child process started\n");

    // Ждём завершения дочернего процесса
    waitpid(pid, NULL, 0);

    // Эта строка гарантированно появится после завершения cat
    printf("Parent: child process finished\n");

    return 0;
}
