#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void print_uids(const char *label)
{
    printf("%s\n", label);
    printf("Real UID:     %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
}

int main(void)
{
    FILE *f;

    print_uids("UIDs before setuid():");

    f = fopen("data.txt", "r");
    if (f == NULL) {
        perror("fopen data.txt (before setuid)");
    } else {
        printf("data.txt opened successfully (before setuid)\n");
        fclose(f);
    }

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return EXIT_FAILURE;
    }

    print_uids("UIDs after setuid():");

    f = fopen("data.txt", "r");
    if (f == NULL) {
        perror("fopen data.txt (after setuid)");
    } else {
        printf("data.txt opened successfully (after setuid)\n");
        fclose(f);
    }

    return EXIT_SUCCESS;
}
