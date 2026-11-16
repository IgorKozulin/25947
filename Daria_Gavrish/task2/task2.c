#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main()
{
    time_t now;
    struct tm *sp;

    // Калифорния
    putenv("TZ=PST8");
    tzset();

    (void) time( &now );

    printf("Californian time:\n");
    printf("%s", ctime( &now ) );

    sp = localtime(&now);
    printf("Formatted: %d/%d/%02d %d:%02d:%02d %s\n\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year, sp->tm_hour,
        sp->tm_min, sp->tm_sec, tzname[sp->tm_isdst]);

    //Москва 
    putenv("TZ=MSK-3");
    tzset();

    printf("Moscow time:\n");
    printf("%s", ctime( &now ));

    sp = localtime(&now);
    printf("Formatted: %d/%d/%02d %d:%02d:%02d %s\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year, sp->tm_hour,
        sp->tm_min, sp->tm_sec, tzname[sp->tm_isdst]);

    exit(0);
}