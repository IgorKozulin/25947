#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

int main(void) {
    int pipefd[2];
    pid_t pid;
    char buffer[1024];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(pipefd[1]);
        printf("[CHILD] Запущен. Ожидаю данные от родителя...\n");
        fflush(stdout);

        ssize_t n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            int changed_count = 0;

            for (int i = 0; i < n; i++) {
                char orig = buffer[i];
                buffer[i] = toupper((unsigned char)buffer[i]);
                if (orig != buffer[i]) {
                    changed_count++;
                }
            }
            
            printf("[CHILD] Получено и преобразовано: %s", buffer);
            printf("[CHILD] Изменено символов: %d\n", changed_count);
            fflush(stdout);
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);

    } else {
        close(pipefd[0]);
        printf("[PARENT] Связь через канал установлена!\n");
        printf("[PARENT] Введите текст (пустая строка или Ctrl+D — выход):\n");

        while (1) {
            printf("> ");
            fflush(stdout);
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
            if (strcmp(buffer, "\n") == 0) break;
            write(pipefd[1], buffer, strlen(buffer));
        }

        close(pipefd[1]);
        wait(NULL);
        printf("[PARENT] Потомок завершился. Родитель тоже завершает работу.\n");
    }

    return 0;
}
