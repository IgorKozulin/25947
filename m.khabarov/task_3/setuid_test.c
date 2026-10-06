#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());

    FILE *f = fopen("data.txt", "r");
    if (f) {
        printf("File opened successfully (before setuid reset)\n");
        fclose(f);
    } else {
        perror("fopen failed (before setuid reset)");
    }

    setuid(getuid());

    printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());

    f = fopen("data.txt", "r");
    if (f) {
        printf("File opened successfully (after setuid reset)\n");
        fclose(f);
    } else {
        perror("fopen failed (after setuid reset)");
    }

    return 0;
}