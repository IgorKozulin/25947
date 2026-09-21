#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(void)
{
    const char *filename = "data.txt";
    FILE *f;

    printf("Before setuid:\n");
    printf("  real uid      = %d\n", getuid());
    printf("  effective uid = %d\n", geteuid());

    f = fopen(filename, "r");
    if (f == NULL) {
        perror("fopen");
    } else {
        printf("  fopen(%s): OK\n", filename);
        fclose(f);
    }

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return 1;
    }

    printf("\nAfter setuid:\n");
    printf("  real uid      = %d\n", getuid());
    printf("  effective uid = %d\n", geteuid());

    f = fopen(filename, "r");
    if (f == NULL) {
        perror("fopen");
    } else {
        printf("  fopen(%s): OK\n", filename);
        fclose(f);
    }

    return 0;
}
