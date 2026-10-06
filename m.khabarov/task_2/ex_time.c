#include <stdio.h>        // printf
#include <time.h>         // time_t, struct tm, time(), localtime()
#include <stdlib.h>       // setenv()

int main() {
    // 1. Установка часового пояса Калифорнии
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();  // Применяем изменения
    
    // 2. Получение текущего времени (UTC)
    time_t now;
    time(&now);
    
    // 3. Конвертация в локальное время с учетом выбранного пояса
    struct tm *sp;
    sp = localtime(&now);
    
    // 4. Форматированный вывод
    // Формат: ДД/ММ/ГГГГ ЧЧ:ММ Название_пояса
    printf("%02d/%02d/%04d %02d:%02d %s\n",
           sp->tm_mday,              // день месяца
           sp->tm_mon + 1,           // месяц (0-11, поэтому +1)
           sp->tm_year + 1900,       // год (от 1900, поэтому +1900)
           sp->tm_hour,              // часы
           sp->tm_min,               // минуты
           tzname[sp->tm_isdst]);    // название пояса (PST или PDT)
    
    return 0;
}