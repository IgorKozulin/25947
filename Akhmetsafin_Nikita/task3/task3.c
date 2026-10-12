#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

void print_ids() {
    printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
}

void try_open_file() {
    FILE *file = fopen("data.txt", "r");
    if (file != NULL) {
        printf("Success: File data.txt opened.\n");
        fclose(file);
    } else {
        perror("fopen error");
    }
}

int main() {
    print_ids();
    try_open_file();

    printf("\n--- Calling setuid(getuid()) ---\n");
    if (setuid(getuid()) == -1) {
        perror("setuid error");
    }

    print_ids();
    try_open_file();

    return 0;
}
