#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
extern char *tzname[];
 
int main(void)
{
    time_t now;
    struct tm *sp;
 
    setenv("TZ", "PST8PDT", 1); // Функция для изменения переменной окружения. 1 — перезаписать, если TZ уже задана
    // PST — имя пояса в стандартное время
    // 8 — смещение от UTC в часах, на запад (без минуса, т.к. пояс западнее UTC)
    // PDT — имя пояса на время действия летнего перехода
 
    tzset(); // пересчитать timezone, tzname, altzone
 
    (void) time( &now );
 
    printf("%s", ctime( &now ) );
 
    sp = localtime(&now);
 
    // tzname[sp->tm_isdst] сам выберет PST или PDT
    // в зависимости от того, действует ли летнее время на дату now
    printf("%d/%d/%02d %d:%02d %s\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year, sp->tm_hour,
        sp->tm_min, tzname[sp->tm_isdst]);
    exit(0);
}

