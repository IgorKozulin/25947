#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(void) {
    // 1. Установка часового пояса
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();  // применяем настройки

    // 2. Получение текущего времени
    time_t now;
    time(&now);

    // 3. Конвертация в локальное время
    struct tm *sp = localtime(&now);

    // 4. Форматированный вывод
    printf("%02d/%02d/%04d %02d:%02d %s\n",
           sp->tm_mday,          // день месяца
           sp->tm_mon + 1,       // месяц (0-11 -> 1-12)
           sp->tm_year + 1900,   // год (от 1900)
           sp->tm_hour,          // часы
           sp->tm_min,           // минуты
           tzname[sp->tm_isdst]); // PST или PDT

    return 0;
}
