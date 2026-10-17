#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

void print_uid(){
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
}

void check_file()
{
    FILE *file = fopen("data.txt", "r");

    if (file == NULL)
    {
        perror("fopen");
    }

    else
    {
        printf("successfully\n");
        fclose(file);
    }
}

int main(void)
{
    setenv("TZ", "Asia/Novosibirsk",1);
                    tzset();
                    time_t current_time;
                    struct tm *sp;

                    time(&current_time);
                    sp = localtime(&current_time);

                    printf("%02d/%02d/%04d %02d:%02d %s\n", 
                            sp -> tm_mday,
                            sp -> tm_mon + 1,
                            sp -> tm_year + 1900,
                            sp -> tm_hour,
                            sp -> tm_min,
                            tzname[sp -> tm_isdst]);
    print_uid();
    check_file();

    if (setuid(getuid()) == -1)
    {
        perror("setuid");
        return 1;
    }
    print_uid();
    check_file();
    return 0;
}
