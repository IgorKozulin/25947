#include <stdio.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h> 

int main(int argc, char *argv[])
{
    pid_t pid;   //храниться идентификатор дочернего процесса
    int status; // статус завершения дочернего процесса

    if (argc != 2) {    
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;    
    }

    pid = fork();// Создаём дочерний процесс

    if (pid == -1) { 
        perror("fork"); 
        return 1;    
    }

    if (pid == 0) { // Этот блок выполняется только в дочернем процессе
        printf("[CHILD] Printing file:\n"); // Ребёнок сообщает, что будет выводить файл
        fflush(stdout); // Сразу выводит текст из буфера на экран
        execlp("cat", "cat", argv[1], NULL); // Заменяет ребёнка программой cat и передаёт ей имя файла
        perror("execlp"); // Выполнится только если команду cat не удалось запустить
        return 1; // Завершаем дочерний процесс с ошибкой
    }

    printf("[PARENT] Child PID: %d\n", pid); // Родитель выводит номер созданного ребёнка
    printf("[PARENT] I am working while child prints the file.\n"); // Родитель печатает свой текст

    if (waitpid(pid, &status, 0) == -1) { // Родитель ждёт завершения конкретного ребёнка
        perror("waitpid");// Печатаем ошибку ожидания процесса.
        return 1;// Завершаем программу с ошибкой
    }

    if (WIFEXITED(status)) { // Проверяем, завершился ли ребёнок обычным способом
        printf("[PARENT] Child exit code: %d\n", WEXITSTATUS(status)); // Печатаем код завершения ребёнка
    }

    printf("[PARENT] Child has finished. This is the last parent line.\n"); // Эта строка выводится после cat

    return 0; // Нормально завершаем родительскую программу
}
