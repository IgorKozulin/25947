#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

static void check_file(void) {
    printf("UID=%lu EUID=%lu\n",
           (unsigned long)getuid(),
           (unsigned long)geteuid());
    fflush(stdout);

    FILE *file = fopen("data.txt", "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    puts("File opened successfully");
    if (fclose(file) == EOF) {
        perror("fclose");
    }
}

int main(void) {
    puts("Before setuid:");
    check_file();

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return 1;
    }

    puts("After setuid:");
    check_file();

    return 0;
}
