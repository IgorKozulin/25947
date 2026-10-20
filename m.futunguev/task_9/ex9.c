/* Задание 9. Создание двух процессов.
 * Потомок исполняет cat(1) файла, родитель печатает текст.
 * 1: родитель просто печатает (вывод перемешивается с cat).
 * 2: последняя строка родителя печатается после wait.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Использование: %s файл\n", argv[0]);
        return 1;
    }

    pid_t pid = fork(); // fork() - создаёт копию текущего процесса.
    if (pid < 0) { // если процесс создать не удалось   
        perror("fork");
        return 1;
    }
    if (pid == 0) {                       /* потомок */
        execlp("cat", "cat", argv[1], (char *)NULL); // Заменяем программу процесса на cat
        perror("execlp");                 /* сюда попадём только при ошибке */
        _exit(127);
    }

    /* родитель: текст выводится сразу, не дожидаясь потомка */
    printf("Родитель: запущен потомок с pid %d\n", (int)pid);
    fflush(stdout); // Принудительно выталкивает буфер вывода на экран.

    /* ждём завершения потомка */
    int status; // информацию о том, как завершился потомок.
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    printf("Родитель: потомок завершился - это последняя строка\n");
    return 0;
}