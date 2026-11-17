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

    /* 1. Создаем канал */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    /* 2. Создаем подпроцесс */
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* === ЛОГИКА ДОЧЕРНЕГО ПРОЦЕССА === */
        close(pipefd[1]); /* Закрываем конец записи (нам он не нужен) */
        
        printf("[CHILD] Запущен. Ожидаю данные от родителя...\n");
        fflush(stdout);

        ssize_t n;
        /* Читаем из канала, пока не вернется 0 (EOF) */
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0'; /* Добавляем нуль-терминатор для printf */
            
            /* Переводим в верхний регистр */
            for (int i = 0; i < n; i++) {
                buffer[i] = toupper((unsigned char)buffer[i]);
            }
            
            printf("[CHILD] Получено и преобразовано: %s", buffer);
            fflush(stdout); /* Обязательно сбрасываем буфер, чтобы вывод появился сразу */
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);

    } else {
        /* === ЛОГИКА РОДИТЕЛЬСКОГО ПРОЦЕССА === */
        close(pipefd[0]); /* Закрываем конец чтения (нам он не нужен) */

        printf("[PARENT] Связь через канал установлена!\n");
        printf("[PARENT] Введите текст (пустая строка или Ctrl+D — выход):\n");

        while (1) {
            printf("> ");
            fflush(stdout);

            /* Читаем строку с клавиатуры */
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                break; /* Ctrl+D (EOF) */
            }

            /* Если ввели пустую строку (просто Enter) — выходим */
            if (strcmp(buffer, "\n") == 0) {
                printf("[PARENT] Пустая строка — завершаю ввод.\n");
                break;
            }

            /* Отправляем строку в канал */
            if (write(pipefd[1], buffer, strlen(buffer)) == -1) {
                perror("write");
                break;
            }
        }

        /* Закрываем конец записи, чтобы ребенок получил EOF и завершился */
        close(pipefd[1]);
        
        /* Ждем завершения ребенка */
        wait(NULL);
        printf("[PARENT] Потомок завершился. Родитель тоже завершает работу.\n");
    }

    return 0;
}
