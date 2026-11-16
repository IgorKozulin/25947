
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

int add_line(LineInfo **table, size_t *count, size_t *capacity,
             off_t start, off_t end)
{
    if (*count == *capacity) {
        size_t new_capacity = *capacity == 0 ? 8 : *capacity * 2;
        LineInfo *temp = realloc(*table, new_capacity * sizeof(LineInfo));

        if (temp == NULL)
            return -1;

        *table = temp;
        *capacity = new_capacity;
    }

    (*table)[*count].offset = start;
    (*table)[*count].length = (size_t)(end - start);
    (*count)++;

    return 0;
}

int main(int argc, char *argv[])
{
    int fd;
    int status = 0;
    LineInfo *table = NULL;
    size_t count = 0, capacity = 0;
    off_t start = 0;
    char ch;
    ssize_t bytes;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    while (1) {
        bytes = read(fd, &ch, 1);

        if (bytes < 0) {
            if (errno == EINTR)
                continue;

            perror("read");
            status = 1;
            goto finish;
        }

        if (bytes == 0 || ch == '\n') {
            off_t end = lseek(fd, 0, SEEK_CUR);

            if (end == (off_t)-1) {
                perror("lseek");
                status = 1;
                goto finish;
            }

            if (end > start) {
                if (add_line(&table, &count, &capacity, start, end) == -1) {
                    perror("realloc");
                    status = 1;
                    goto finish;
                }
            }

            start = end;

            if (bytes == 0)
                break;
        }
    }

    printf("--- Debug: Line Table ---\n");

    for (size_t i = 0; i < count; i++) {
        printf("Line %zu: Offset = %lld, Length = %zu\n",
               i + 1, (long long)table[i].offset, table[i].length);
    }

    printf("-------------------------\n");

    while (1) {
        char input[128];
        long number;
        char extra;

        printf("Enter line number (0 to quit): ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        if (sscanf(input, "%ld %c", &number, &extra) != 1) {
            printf("Invalid input\n");
            continue;
        }

        if (number == 0)
            break;

        if (number < 1 || (unsigned long)number > count) {
            printf("Line does not exist\n");
            continue;
        }

        LineInfo info = table[number - 1];
        char *buffer = malloc(info.length + 1);

        if (buffer == NULL) {
            perror("malloc");
            status = 1;
            goto finish;
        }

        if (lseek(fd, info.offset, SEEK_SET) == (off_t)-1) {
            perror("lseek");
            free(buffer);
            status = 1;
            goto finish;
        }

        size_t total = 0;

        while (total < info.length) {
            bytes = read(fd, buffer + total, info.length - total);

            if (bytes < 0 && errno == EINTR)
                continue;

            if (bytes <= 0) {
                if (bytes == -1)
                    perror("read");
                else
                    fprintf(stderr, "Unexpected end of file\n");

                free(buffer);
                status = 1;
                goto finish;
            }

            total += (size_t)bytes;
        }

        buffer[info.length] = '\0';

        printf("%s", buffer);

        if (buffer[info.length - 1] != '\n')
            putchar('\n');

        free(buffer);
    }

finish:
    free(table);

    if (close(fd) == -1) {
        perror("close");
        status = 1;
    }

    return status;
}

