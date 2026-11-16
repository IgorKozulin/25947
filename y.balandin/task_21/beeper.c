#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/* Глобальный счётчик SIGINT.
   volatile     — компилятор не кэширует значение в регистре;
   sig_atomic_t — чтение/запись атомарны. */
static volatile sig_atomic_t beep_count = 0;

/* Обработчик SIGINT (Ctrl+C): пикнуть и увеличить счётчик. */
static void sigint_handler(int sig)
{
    (void)sig;                      // параметр не нужен
    write(STDOUT_FILENO, "\a", 1);  // \a = ASCII 7 = звуковой сигнал
    beep_count++;                   // атомарный инкремент
}

/* Обработчик SIGQUIT (Ctrl+\): вывести счётчик и завершиться. */
static void sigquit_handler(int sig)
{
    (void)sig;

    char buffer[256];               // локальный буфер для сообщения
    int len = snprintf(buffer, sizeof(buffer), "\nВсего прозвучало сигналов: %d\n" "Программа завершена по SIGQUIT\n", (int)beep_count);  // форматируем в буфер

    if (len > 0)                    // если строка сформирована
        write(STDOUT_FILENO, buffer, (size_t)len);  // безопасный вывод

    _exit(EXIT_SUCCESS);            // _exit, а не exit: без очистки stdio
}

int main(void)
{
    struct sigaction sa_int, sa_quit;  // структуры для настройки сигналов

    /* --- Настройка SIGINT --- */
    sa_int.sa_handler = sigint_handler;  // функция-обработчик
    sigemptyset(&sa_int.sa_mask);        // не блокировать доп. сигналы
    sa_int.sa_flags = SA_RESTART;        // перезапускать прерванные вызовы
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {  // установить обработчик
        perror("sigaction SIGINT");
        return EXIT_FAILURE;
    }

    /* --- Настройка SIGQUIT --- */
    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;                // флаги не нужны: процесс завершится
    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        perror("sigaction SIGQUIT");
        return EXIT_FAILURE;
    }
    printf("Пищалка запущена!\n");
    printf("Нажмите Ctrl+C — будет звуковой сигнал\n");
    printf("Нажмите Ctrl+\\ — программа завершится с подсчётом сигналов\n");
    printf("Ожидаю сигналов...\n");
    fflush(stdout);  // сбросить буфер, чтобы текст появился сразу

    /* --- Основной цикл --- */
    while (1)
        pause();     // спать до любого сигнала, не грузя CPU

    return EXIT_SUCCESS;  // недостижимо
}
