#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

int fd;

struct LineInfo {
    long offset;
    int length;
};

void alarm_handler(int sig)
{
    char c;

    lseek(fd, 0L, 0);

    while (read(fd, &c, 1) > 0)
    {
        write(1, &c, 1);
    }

    close(fd);
    _exit(0);
}

int main(int args, char **argv)
{
    fd = open(argv[1], O_RDONLY);

    struct LineInfo table[100];
    int lines = 0;

    char c;
    long start = 0;

    while (read(fd, &c, 1) > 0)
    {
        if (c == '\n')
        {
            long position = lseek(fd, 0L, 1);

            table[lines].offset = start;
            table[lines].length = position - start;

            lines++;
            start = position;
        }
    }

    for (int i = 0; i < lines; i++)
    {
        printf("Line %d: offset = %ld, length = %d\n",
               i + 1, table[i].offset, table[i].length);
    }

    signal(SIGALRM, alarm_handler);

    int number;

    while (1)
    {
        printf("Enter line number (0 to quit): ");
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

        lseek(fd, table[index].offset, 0);

        char *buffer = malloc(table[index].length + 1);

        read(fd, buffer, table[index].length);
        buffer[table[index].length] = '\0';

        printf("%s", buffer);

        free(buffer);
    }

    close(fd);

    return 0;
}
