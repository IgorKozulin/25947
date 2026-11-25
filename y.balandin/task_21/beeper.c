#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/* Глобальные переменные */
static volatile sig_atomic_t beep_count = 0;

/* Обработчик SIGINT (Ctrl+C): короткий писк-подтверждение и счётчик */
static void sigint_handler(int sig)
{
    (void)sig;
    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

/* Обработчик SIGQUIT (Ctrl+\): статистика и выход */
static void sigquit_handler(int sig)
{
    (void)sig;
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
                       "\nВсего прозвучало сигналов: %d\n"
                       "Программа завершена по SIGQUIT\n",
                       (int)beep_count);
    if (len > 0)
        write(STDOUT_FILENO, buffer, (size_t)len);
    _exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <файл.txt>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    /* Настройка сигналов */
    struct sigaction sa_int, sa_quit;

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        fclose(fp);
        return EXIT_FAILURE;
    }

    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;
    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        perror("sigaction SIGQUIT");
        fclose(fp);
        return EXIT_FAILURE;
    }

    printf("Пищалка-Морзе запущена!\n");
    printf("Файл: %s\n", argv[1]);
    printf("'-' — звук, '.' — тишина\n");
    printf("Ctrl+C — писк, Ctrl+\\ — выход\n\n");

    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '-') {
            write(STDOUT_FILENO, "\a", 1);  // звук
            beep_count++;
            usleep(200000);                  // 200 мс звука
        } else if (c == '.') {
            usleep(1000000);                  // 200 мс тишины
        }
        // остальные символы (\n, пробелы) просто пропускаем
    }

    fclose(fp);
    printf("\nФайл прочитан. Ожидаю Ctrl+\\ для выхода...\n");
    fflush(stdout);

    while (1)
        pause();

    return EXIT_SUCCESS;
}
