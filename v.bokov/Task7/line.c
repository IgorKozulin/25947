
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

struct LineInfo {
    long pos;
    int len;
};

char *file;
size_t size;

void alarm_handler(int sig)
{
    (void)sig;

    const char msg[] = "\nTime is up!\n";

    fwrite(msg, 1, sizeof(msg) - 1, stdout);
    fwrite(file, 1, size, stdout);

    if (size > 0 && file[size - 1] != '\n')
    {
        fwrite("\n", 1, 1, stdout);
    }

    fflush(stdout);
    _exit(0);
}

int main(int args, char **argv)
{
    if (args < 2)
    {
        printf("Enter filename\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        perror("fstat");
        close(fd);
        return 1;
    }

    size = st.st_size;
    printf("File size: %zu bytes\n", size);

    if (size == 0)
    {
        printf("File is empty.\n");
        close(fd);
        return 0;
    }

    file = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (file == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    close(fd);

    struct LineInfo table[100];
    int lines = 0;
    long start = 0;

    for (size_t i = 0; i < size; i++)
    {
        if (file[i] == '\n')
        {
            if (lines >= 100)
            {
                printf("Too many lines\n");
                munmap(file, size);
                return 1;
            }

            table[lines].pos = start;
            table[lines].len = i + 1 - start;

            lines++;
            start = i + 1;
        }
    }

    if ((size_t)start < size)
    {
        if (lines >= 100)
        {
            printf("Too many lines\n");
            munmap(file, size);
            return 1;
        }

        table[lines].pos = start;
        table[lines].len = size - start;
        lines++;
    }

    for (int i = 0; i < lines; i++)
    {
        printf("Line %d: pos = %ld, len = %d\n",
               i + 1, table[i].pos, table[i].len);
    }

    signal(SIGALRM, alarm_handler);

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

        fwrite(file + table[index].pos,
               1, table[index].len, stdout);

        if (table[index].len == 0 ||
            file[table[index].pos + table[index].len - 1] != '\n')
        {
            printf("\n");
        }
    }

    munmap(file, size);

    return 0;
}
