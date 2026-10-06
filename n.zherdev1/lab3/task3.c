#include <stdio.h>
#include <unistd.h>

void check(const char *file)
{
    printf("real uid: %d, effective uid: %d\n", getuid(), geteuid());

    FILE *f = fopen(file, "r");

    if (f == NULL)
        perror("fopen");
    else
        fclose(f);
}

int main(void)
{
    check("data.txt");

    if (setuid(getuid()) == -1)
        perror("setuid");

    check("data.txt");

    return 0;
}
