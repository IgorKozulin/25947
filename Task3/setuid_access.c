#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

static void
print_ids(const char *stage)
{
    printf("%s: real uid=%lu, effective uid=%lu\n", stage,
        (unsigned long)getuid(), (unsigned long)geteuid());
}

static void
try_open_file(const char *path, const char *stage)
{
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        printf("%s: ", stage);
        perror(path);
        return;
    }

    printf("%s: opened %s successfully\n", stage, path);
    if (fclose(file) == EOF) {
        perror("fclose");
    }
}

int
main(int argc, char **argv)
{
    const char *path = "private_data.txt";
    uid_t real_uid;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [data-file]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2) {
        path = argv[1];
    }

    real_uid = getuid();
    print_ids("before setuid");
    try_open_file(path, "before setuid");

    if (setuid(real_uid) == -1) {
        perror("setuid(real uid)");
        return EXIT_FAILURE;
    }

    print_ids("after setuid");
    try_open_file(path, "after setuid");
    return EXIT_SUCCESS;
}
