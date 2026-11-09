#include <stdio.h>
#include <stdlib.h>
#include <signal.h>     // sigaction, SIGINT, SIGQUIT
#include <unistd.h>     // write, pause, _exit

volatile sig_atomic_t count = 0; // сколько раз пищали

// обработчик SIGINT (Ctrl+C): пищим и считаем
void on_int(int sig)
{
    (void)sig;          // номер сигнала не используем
    write(1, "\a", 1);  // звуковой сигнал
    count++;
}

// обработчик SIGQUIT (Ctrl+\): печатаем результат и выходим
void on_quit(int sig)
{
    (void)sig;
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "\nbeeped %d times\n", (int)count);
    write(1, msg, len); // write безопасен внутри обработчика, printf - нет
    _exit(0);           // _exit безопасен внутри обработчика, exit - нет
}

// обработчик SIGALRM - 5 секунд ничего не нажимали, выходим
void on_alarm(int sig)
{
    (void)sig;
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "\ntime out, beeped %d times\n", (int)count);
    write(1, msg, len);
    _exit(0);
}


int main(void)
{
    // struct sigaction - структура, в которой описываем, как реагировать на сигнал:
    // какую функцию вызвать, какие сигналы блокировать, какие флаги включить
    struct sigaction sa;

    // назначаем обработчик для SIGINT (Ctrl+C)
    // sa_handler - функция, которую система вызовет при приходе сигнала.
    sa.sa_handler = on_int;

    // sa_mask - набор сигналов, которые блокируются, пока работает обработчик.
    // sigemptyset делает набор пустым: во время обработчика ничего не блокируем
    sigemptyset(&sa.sa_mask);

    // sa_flags - дополнительные настройки. 0 - обычное поведение, без особых флагов
    sa.sa_flags = 0;

    // sigaction применяет настройки: "при SIGINT вызывай on_int".
    // Третий аргумент NULL - старые настройки нам не нужны, не сохраняем их.
    // В отличие от signal(), обработчик остаётся назначенным после каждого
    // срабатывания (на Solaris signal() сбрасывал бы его после первого Ctrl+C)
    sigaction(SIGINT, &sa, NULL);

    // назначаем обработчик для SIGQUIT (Ctrl+\)

    // используем ту же структуру: маска и флаги уже заданы выше,
    // меняем только функцию-обработчик
    sa.sa_handler = on_quit;

    // "при SIGQUIT вызывай on_quit"
    sigaction(SIGQUIT, &sa, NULL);

    // подсказка пользователю, какие клавиши нажимать.
    // \\ - так в строке C пишется один символ обратной косой черты
    printf("Ctrl+C - beep, Ctrl+\\ - quit, 5 sec without Ctrl+C - exit\n"); // изменил: + про 5 секунд
 
    alarm(5); // добавил: запускаем таймер, через 5 секунд придёт SIGALRM
 


    // бесконечный цикл: программа живёт, пока не придёт SIGQUIT
    while (1)
        pause(); // pause() усыпляет процесс до прихода любого сигнала.
                 // Пришёл SIGINT - выполнился on_int, pause() вернулся,
                 // цикл снова вызывает pause() и программа опять спит.
                 // Пришёл SIGQUIT - on_quit вызывает _exit, и программа завершается

    return 0; // сюда программа никогда не дойдёт, но main по правилам возвращает int
}