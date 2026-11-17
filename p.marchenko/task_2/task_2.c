#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Таблица смещений от UTC в секундах.
   Смещение = локальное время минус UTC. */
struct zone {
    const char *name;
    int offset_sec;
};

static struct zone zones[] = {
    { "PST",  -8 * 3600 },   /* Pacific Standard Time */
    { "PDT",  -7 * 3600 },   /* Pacific Daylight Time */
    { "MSK",   3 * 3600 },   /* Moscow */
    { "UTC",   0 },
    { "GMT",   0 },
    { "EST",  -5 * 3600 },
    { "CET",   1 * 3600 },
    { NULL, 0 }
};

int main(int argc, char *argv[]) {
    const char *zonename = "PST";   /* по умолчанию — как в задании */
    int offset = -8 * 3600;

    if (argc > 1) {
        zonename = argv[1];
        int found = 0;
        for (struct zone *z = zones; z->name; ++z) {
            if (strcmp(z->name, zonename) == 0) {
                offset = z->offset_sec;
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Unknown zone: %s\n", zonename);
            fprintf(stderr, "Known zones: PST, PDT, MSK, UTC, GMT, EST, CET\n");
            return 1;
        }
    }

    time_t t = time(NULL);
    t += offset;
    struct tm *tm = gmtime(&t);

    char buf[128];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    printf("%s %s\n", buf, zonename);
    return 0;
}
