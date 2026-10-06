#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void print_uids()
{
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
}

void test_file()
{
    FILE *file;

    file = fopen("data.txt", "r");

    if (file != NULL)
    {
        printf("data.txt: access granted\n");
        fclose(file);
    }
    else
    {
        perror("data.txt");
    }
}

int main()
{
    printf("Before setuid:\n");
    print_uids();

    printf("File access:\n");
    test_file();

    if (setuid(getuid()) == -1)
    {
        perror("setuid");
        return EXIT_FAILURE;
    }

    printf("\nAfter setuid(getuid()):\n");
    print_uids();

    printf("File access:\n");
    test_file();

    return EXIT_SUCCESS;
}
