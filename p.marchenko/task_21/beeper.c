#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

/* Глобальная переменная для безопасного подсчета в обработчике */
volatile sig_atomic_t beep_count = 0;

/* Обработчик SIGINT (Ctrl+C) */
void sigint_handler(int sig) {
    (void)sig; /* Убираем предупреждение о неиспользуемом параметре */
    /* Безопасный вывод звукового сигнала (ASCII 7) */
    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

/* Обработчик SIGQUIT (Ctrl+\) */
void sigquit_handler(int sig) {
    (void)sig;
    char buffer[128];
    /* Формируем сообщение. snprintf безопаснее, чем sprintf */
    int len = snprintf(buffer, sizeof(buffer), 
                       "\nВсего прозвучало сигналов: %d\nПрограмма завершена по SIGQUIT\n", 
                       beep_count);
    /* Выводим через write, так как printf внутри обработчика использовать нельзя */
    write(STDOUT_FILENO, buffer, len);
    _exit(EXIT_SUCCESS);
}

int main(void) {
    struct sigaction sa_int, sa_quit;

    /* --- Настройка SIGINT --- */
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART; /* Перезапускать pause() после сигнала */
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(1);
    }

    /* --- Настройка SIGQUIT --- */
    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0; /* Флаги не важны, так как мы завершаем процесс */
    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        perror("sigaction SIGQUIT");
        exit(1);
    }

    /* --- Отключение вывода ^C на экран (ECHOCTL) --- */
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) == 0) {
        term.c_lflag &= ~ECHOCTL; /* Убираем флаг */
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
    }

    printf("Пищалка запущена!\n");
    printf("Нажмите Ctrl+C — будет звуковой сигнал\n");
    printf("Нажмите Ctrl+\\ — программа завершится с подсчетом сигналов\n");
    printf("Ожидаю сигналов...\n");

    /* Бесконечный цикл. pause() засыпает и просыпается только при получении сигнала */
    while (1) {
        pause();
    }

    return 0;
}
