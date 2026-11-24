#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    FILE *file;
    int ch;
    int count = 0;

    if (argc < 2)
    {
        printf("Usage: %s file.txt\n", argv[0]);
        return 1;
    }

    file = fopen(argv[1], "r");

    if (file == NULL)
    {
        perror("fopen");
        return 1;
    }

    printf("Reading file: %s\n", argv[1]);

    while ((ch = fgetc(file)) != EOF)
    {
        if (ch == '-')
        {
            write(STDOUT_FILENO, "\a", 1);
            printf("- Beep!\n");
            fflush(stdout);

            count++;
            sleep(1);
        }
        else if (ch == '.')
        {
            printf(". Silence\n");
            fflush(stdout);

            sleep(1);
        }
    }

    fclose(file);

    printf("Total beeps: %d\n", count);
    printf("Program finished.\n");

    return 0;
}
