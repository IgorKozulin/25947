#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main()
{
    time_t now;
    time_t pst;
    struct tm *sp;

    (void) time(&now);
    
    pst = now - (8 * 60 * 60);
    sp = gmtime(&pst);
    
    printf("California Time: %s", asctime(sp));
    
    printf("Formatted: %d/%d/%02d %d:%02d PST\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year, sp->tm_hour,
        sp->tm_min);

    exit(0);
}

