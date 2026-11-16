#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

int main(void)
{
    int pipefd[2];
    pid_t pid;

    // 1. Создаем канал
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 2. Создаем дочерний процесс
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // 3. ЛОГИКА ДОЧЕРНЕГО ПРОЦЕССА (ЧИТАТЕЛЬ)
    if (pid == 0) {
        close(pipefd[1]); // Закрываем конец записи, он нам не нужен

        printf("[CHILD] Запущен. Ожидаю данные от родителя...\n");
        fflush(stdout);

        char buffer[1024];
        ssize_t bytes_read;

        // Читаем из канала, пока родитель не закроет свой конец записи (EOF)
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0'; // Добавляем нуль-терминатор для безопасной работы

            // Переводим каждый символ в верхний регистр
            for (int i = 0; i < bytes_read; i++) {
                // Важно: приводим к unsigned char, как сказано в задании
                buffer[i] = toupper((unsigned char)buffer[i]);
            }

            printf("[CHILD] Получено и преобразовано: %s", buffer);
            fflush(stdout); // Обязательно сбрасываем буфер, чтобы увидеть текст сразу
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    }
    // 4. ЛОГИКА РОДИТЕЛЬСКОГО ПРОЦЕССА (ПИСАТЕЛЬ)
    else {
        close(pipefd[0]); // Закрываем конец чтения, он нам не нужен

        printf("[PARENT] Связь через канал установлена!\n");
        printf("[PARENT] Введите текст (пустая строка или Ctrl+D — выход):\n");
        fflush(stdout);

        char line[1024];
        while (1) {
            printf("> ");
            fflush(stdout);

            // Читаем строку с клавиатуры
            if (fgets(line, sizeof(line), stdin) == NULL) {
                // Если нажат Ctrl+D (EOF)
                printf("\n[PARENT] EOF — завершаю ввод.\n");
                break;
            }

            // Если введена пустая строка (просто нажат Enter)
            if (line[0] == '\n') {
                printf("[PARENT] Пустая строка — завершаю ввод.\n");
                break;
            }

            // Отправляем строку в канал
            if (write(pipefd[1], line, strlen(line)) == -1) {
                perror("write");
                break;
            }
        }

        // Закрываем конец записи. Это даст дочернему процессу сигнал EOF (read вернет 0).
        close(pipefd[1]); 

        // Ждем завершения потомка
        wait(NULL);
        printf("[PARENT] Потомок завершился. Родитель тоже завершает работу.\n");
    }

    return EXIT_SUCCESS;
}
