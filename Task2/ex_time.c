#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int
main(void)
{
    time_t now;
    struct tm california_time;
    char formatted[64];

    /*
     * PST8 is a fixed UTC-8 offset. The assignment asks for PST rather than
     * the daylight-saving-aware America/Los_Angeles timezone.
     */
    if (setenv("TZ", "PST8", 1) == -1) {
        perror("setenv(TZ)");
        return EXIT_FAILURE;
    }
    tzset();

    now = time(NULL);
    if (now == (time_t)-1) {
        perror("time");
        return EXIT_FAILURE;
    }
    if (localtime_r(&now, &california_time) == NULL) {
        perror("localtime_r");
        return EXIT_FAILURE;
    }
    if (strftime(formatted, sizeof(formatted),
            "%Y-%m-%d %H:%M:%S %Z", &california_time) == 0) {
        fprintf(stderr, "Could not format the California time\n");
        return EXIT_FAILURE;
    }

    puts(formatted);
    return EXIT_SUCCESS;
}
