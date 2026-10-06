#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    time_t instant;
    struct tm california;
    char result[64];

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [YYYY-MM-DDTHH:MM:SSZ]\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        struct tm utc = {0};
        char *end = strptime(argv[1], "%Y-%m-%dT%H:%M:%SZ", &utc);
        if (end == NULL || *end != '\0') {
            fprintf(stderr, "Invalid UTC date\n");
            return 1;
        }
        /* mktime() interprets tm in the current timezone. */
        if (setenv("TZ", "UTC0", 1) != 0) {
            perror("setenv");
            return 1;
        }
        tzset();
        instant = mktime(&utc);
        if (instant == (time_t)-1) {
            fprintf(stderr, "UTC date is out of range\n");
            return 1;
        }
    } else {
        instant = time(NULL);
    }
    if (setenv("TZ", "America/Los_Angeles", 1) != 0) {
        perror("setenv");
        return 1;
    }
    tzset();
    if (localtime_r(&instant, &california) == NULL ||
        strftime(result, sizeof(result), "%Y-%m-%d %H:%M:%S %Z", &california) == 0) {
        fprintf(stderr, "Cannot format time\n");
        return 1;
    }
    puts(result);
    return 0;
}
