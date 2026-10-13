#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(void)
{
    FILE *file;

    printf("Before setuid():\n");
    printf("Real UID: %d\n", (int)getuid());
    printf("Effective UID: %d\n", (int)geteuid());

    file = fopen("data.txt", "r");

    if (file == NULL)
        perror("fopen");
    else
    {
        printf("File opened successfully.\n");
        fclose(file);
    }

    if (setuid(getuid()) == -1)
    {
        perror("setuid");
        return 1;
    }

    printf("\nAfter setuid():\n");
    printf("Real UID: %d\n", (int)getuid());
    printf("Effective UID: %d\n", (int)geteuid());

    file = fopen("data.txt", "r");

    if (file == NULL)
        perror("fopen");
    else
    {
        printf("File opened successfully.\n");
        fclose(file);
    }

    return 0;
}
