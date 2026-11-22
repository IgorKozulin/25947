#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

volatile sig_atomic_t beep_count = 0;

void sigint_handler(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

void sigquit_handler(int sig) {
    (void)sig;
    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer), "\nВсего прозвучало сигналов: %d\nПрограмма завершена по SIGQUIT\n", beep_count);
    write(STDOUT_FILENO, buffer, len);
    _exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    struct sigaction sa_int, sa_quit;

    /* Настройка сигналов */
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;
    sigaction(SIGQUIT, &sa_quit, NULL);

    /* Если файл не передан, работаем как обычная пищалка */
    if (argc < 2) {
        printf("Пищалка запущена!\n");
        printf("Нажмите Ctrl+C — будет звуковой сигнал\n");
        printf("Нажмите Ctrl+\\ — программа завершится с подсчетом сигналов\n");
        printf("Ожидаю сигналов...\n");
        while(1) pause();
    }

    /* Если файл передан — читаем и воспроизводим морзянку */
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }

    printf("Воспроизвожу Морзе из файла: %s\n", argv[1]);
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '-') {
            /* ТИРЕ: пищим 0.5 секунды */
            for(int i=0; i<5; i++) {
                write(STDOUT_FILENO, "\a", 1);
                usleep(100000); // 0.1 сек
            }
            printf("-"); 
            fflush(stdout);
            usleep(100000);
        } 
        else if (c == '.') {
            /* ТОЧКА: пищит один раз */
            write(STDOUT_FILENO, "\a", 1);
            printf("."); 
            fflush(stdout);
            usleep(300000);
        }
        else if (c == ' ') {
            /* ПРОБЕЛ: пауза */
            usleep(500000);
            printf(" "); 
            fflush(stdout);
        }
    }
    
    fclose(f);
    printf("\nВоспроизведение завершено.\n");
    return 0;
}
