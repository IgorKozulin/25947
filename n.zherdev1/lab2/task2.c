#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    time_t now = time(NULL);

    setenv("TZ", "PST8", 1);
    tzset();

    printf("%s", ctime(&now));

    return 0;
}
