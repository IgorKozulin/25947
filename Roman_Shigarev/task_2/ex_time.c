#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    if (setenv("TZ", "America/Los_Angeles", 1) == -1) {
        perror("setenv");
        return 1;
    }
    tzset();

    time_t now = time(NULL);
    if (now == (time_t)-1) {
        perror("time");
        return 1;
    }

    struct tm *local = localtime(&now);
    if (local == NULL) {
        perror("localtime");
        return 1;
    }

    printf("%02d/%02d/%04d %02d:%02d %s\n",
           local->tm_mday, local->tm_mon + 1,
           local->tm_year + 1900,
           local->tm_hour, local->tm_min,
           tzname[local->tm_isdst > 0]);

    return 0;
}
