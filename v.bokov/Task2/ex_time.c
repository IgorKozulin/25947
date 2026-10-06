#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern char *tzname[2];

int main(void) {
    if (setenv("TZ", "America/Los_Angeles", 1) != 0) {
        perror("setenv error");
        return 1;
    }
    tzset();
    time_t now;
    if (time(&now) == (time_t)-1) {
        perror("time error");
        return 1;
    }


    struct tm *sp = localtime(&now);
    if (sp == NULL) {
        perror("localtime error");
        return 1;
    }

    printf("%02d/%02d/%04d %02d:%02d %s\n",
           sp->tm_mon + 1,
           sp->tm_mday,
           sp->tm_year + 1900,
           sp->tm_hour,
           sp->tm_min,
           tzname[sp->tm_isdst]);

    return 0;
}