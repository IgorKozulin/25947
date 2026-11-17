#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }
    printf("Before setuid: real=%d effective=%d\n", getuid(), geteuid());

    FILE *f = fopen(argv[1], "r");
    if (f) {
        printf("fopen OK\n");
        fclose(f);
    } else {
        perror("fopen");
    }

    if (setuid(getuid()) != 0)
        perror("setuid");

    printf("After setuid: real=%d effective=%d\n", getuid(), geteuid());

    f = fopen(argv[1], "r");
    if (f) {
        printf("fopen OK\n");
        fclose(f);
    } else {
        perror("fopen");
    }
    return 0;
}
