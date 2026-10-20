#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
extern char *tzname[]; // имена пояса: tzname[0] - стандартное время (PST), tzname[1] - летнее (PDT)

int main(void)
{
    time_t now;
    struct tm *sp;

    setenv("TZ", "PST8PDT", 1); // Функция для изменения переменной окружения. 1 — перезаписать, если TZ уже задана
    // PST — имя пояса в стандартное время
    // 8 — смещение от UTC в часах, на запад (без минуса, т.к. пояс западнее UTC)
    // PDT — имя пояса на время действия летнего перехода

    tzset(); // пересчитать timezone, tzname, altzone

    (void) time( &now ); // текущее время в секундах с 1 января 1970 года (UTC), от пояса не зависит

    printf("%s", ctime( &now ) ); // время строкой уже по поясу из TZ; '\n' в конце строки есть

    sp = localtime(&now); // раскладывает время на поля (день, месяц, час...) по поясу из TZ

    // tzname[sp->tm_isdst] сам выберет PST или PDT
    // в зависимости от того, действует ли летнее время на дату now
    printf("%d/%d/%02d %d:%02d:%d %s\n",
        sp->tm_mon + 1, sp->tm_mday, // месяцы в tm считаются с 0, поэтому +1
        sp->tm_year, sp->tm_hour,    // tm_year - число лет с 1900 года
        sp->tm_min, sp->tm_sec, tzname[sp->tm_isdst]);
    exit(0);
}