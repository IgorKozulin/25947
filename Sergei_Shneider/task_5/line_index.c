#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    off_t start;
    off_t length;
} Line;

int main(int argc, char **argv)
{
    int fd, status = 1;
    Line *lines = NULL, *tmp;
    size_t count = 0, number, i;
    off_t start = 0, pos, j;
    char ch;
    ssize_t n;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    while ((n = read(fd, &ch, 1)) == 1) {
        if (ch != '\n') {
            continue;
        }

        pos = lseek(fd, 0L, SEEK_CUR);
        if (pos == (off_t)-1) {
            perror("lseek");
            goto done;
        }

        tmp = realloc(lines, (count + 1) * sizeof(*lines));
        if (!tmp) {
            perror("realloc");
            goto done;
        }
        lines = tmp;

        lines[count].start = start;
        lines[count].length = pos - start;
        ++count;
        start = pos;
    }

    if (n == -1) {
        perror("read");
        goto done;
    }

    pos = lseek(fd, 0L, SEEK_CUR);
    if (pos == (off_t)-1) {
        perror("lseek");
        goto done;
    }

    if (pos > start) {
        tmp = realloc(lines, (count + 1) * sizeof(*lines));
        if (!tmp) {
            perror("realloc");
            goto done;
        }
        lines = tmp;

        lines[count].start = start;
        lines[count].length = pos - start;
        ++count;
    }

    for (i = 0; i < count; ++i) {
        printf("%zu: offset=%lld length=%lld\n", i + 1,
               (long long)lines[i].start,
               (long long)lines[i].length);
    }

    for (;;) {
        printf("Line number (0 to quit): ");
        fflush(stdout);

        if (scanf("%zu", &number) != 1) {
            if (feof(stdin)) {
                status = 0;
                break;
            }
            fputs("Invalid number\n", stderr);
            goto done;
        }

        if (number == 0) {
            status = 0;
            break;
        }

        if (number > count) {
            puts("No such line");
            continue;
        }

        if (lseek(fd, lines[number - 1].start, SEEK_SET)
            == (off_t)-1) {
            perror("lseek");
            goto done;
        }

        for (j = 0; j < lines[number - 1].length; ++j) {
            if (read(fd, &ch, 1) != 1) {
                fputs("Cannot read line\n", stderr);
                goto done;
            }
            printf("%c", ch);
        }

        if (ch != '\n') {
            putchar('\n');
        }
    }

done:
    free(lines);
    if (close(fd) == -1) {
        perror("close");
        return 1;
    }
    return status;
}
