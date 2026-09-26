#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    FILE *fp;
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    fp = fopen("data.txt", "r");
    if (fp == NULL) {
        perror("fopen");
    } else {
        fclose(fp);
    }
    if (setuid(getuid()) == -1) {
        perror("setuid");
        exit(EXIT_FAILURE);
    }
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    fp = fopen("data.txt", "r");
    if (fp == NULL) {
        perror("fopen");
    } else {
        fclose(fp);
    }
    return 0;
}
