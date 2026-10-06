#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

static int show_ids_and_open(const char *path)
{
    FILE *file;
    printf("real UID: %ld; effective UID: %ld\n", (long)getuid(), (long)geteuid());
    file = fopen(path, "r");
    if (file == NULL) {
        perror("fopen");
        return 1;
    }
    puts("open: success");
    if (fclose(file) != 0) {
        perror("fclose");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int failed;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s PRIVATE_FILE\n", argv[0]);
        return 1;
    }
    failed = show_ids_and_open(argv[1]);
    if (setuid(getuid()) == -1) {
        perror("setuid");
        return 1;
    }
    failed |= show_ids_and_open(argv[1]);
    return failed;
}
