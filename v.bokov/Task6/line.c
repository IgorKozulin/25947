#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

struct LineInfo {
    long arr;
    int len;
};

int fd;

void alarm_handler(int sig)
{
    char c;

    (void)sig;

    write(1, "\nTime is up!\n", 13);

    lseek(fd, 0L, 0);

    while (read(fd, &c, 1) > 0)
    {
        write(1, &c, 1);
    }

    _exit(0);
}

int main(int args, char **argv)
{
    if (args < 2)
    {
        printf("Enter filename\n");
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    struct LineInfo table[100];
    int lines = 0;

    char c;
    long start = 0;

    while (read(fd, &c, 1) > 0)
    {
        if (c == '\n')
        {
            long pos = lseek(fd, 0L, 1);

            table[lines].arr = start;
            table[lines].len = pos - start;

            lines++;
            start = pos;
        }
    }

    for (int i = 0; i < lines; i++)
    {
        printf("Line %d: arr = %ld, len = %d\n",
               i + 1, table[i].arr, table[i].len);
    }

    struct sigaction sa = {0};
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    int number;

    while (1)
    {
        printf("number line (5 sec): ");
        fflush(stdout);

        alarm(5);

        if (scanf("%d", &number) != 1)
        {
            alarm(0);
            break;
        }

        alarm(0);  

        if (number == 0)
        {
            break;
        }

        int index = number - 1;

        if (index < 0 || index >= lines)
        {
            printf("Invalid line number\n");
            continue;
        }

        lseek(fd, table[index].arr, 0);

        char *buffer = malloc(table[index].len + 1);

        read(fd, buffer, table[index].len);
        buffer[table[index].len] = '\0';

        printf("%s", buffer);

        free(buffer);
    }

    close(fd);

    return 0;
}
