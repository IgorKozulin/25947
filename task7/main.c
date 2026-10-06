#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "query.h"

struct Line {
    size_t offset;
    size_t length;
};

static int append_line(struct Line **lines, size_t *count, size_t *capacity,
                       size_t offset, size_t length)
{
    if (*count == *capacity) {
        size_t next = *capacity == 0 ? 16 : *capacity * 2;
        struct Line *grown;
        if (next < *capacity || next > SIZE_MAX / sizeof(*grown)) {
            errno = ENOMEM;
            return -1;
        }
        grown = realloc(*lines, next * sizeof(*grown));
        if (grown == NULL)
            return -1;
        *lines = grown;
        *capacity = next;
    }
    (*lines)[(*count)++] = (struct Line){offset, length};
    return 0;
}

int main(int argc, char **argv)
{
    int fd = -1, status = 1;
    struct stat info;
    const char *data = NULL;
    size_t size = 0, count = 0, capacity = 0, start = 0;
    struct Line *lines = NULL;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s TEXT_FILE\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1 || fstat(fd, &info) == -1) {
        perror(argv[1]);
        goto cleanup;
    }
    if (!S_ISREG(info.st_mode) || info.st_size < 0 || (uintmax_t)info.st_size > SIZE_MAX) {
        fprintf(stderr, "Expected a regular file of supported size\n");
        goto cleanup;
    }
    size = (size_t)info.st_size;
    if (size > 0) {
        void *mapping = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapping == MAP_FAILED) {
            perror("mmap");
            goto cleanup;
        }
        data = mapping;
    }
    if (close(fd) == -1) {
        perror("close");
        fd = -1;
        goto cleanup;
    }
    fd = -1;
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '\n') {
            if (append_line(&lines, &count, &capacity, start, i + 1 - start) == -1) {
                perror("realloc");
                goto cleanup;
            }
            start = i + 1;
        }
    }
    if (start < size && append_line(&lines, &count, &capacity, start, size - start) == -1) {
        perror("realloc");
        goto cleanup;
    }
    if (install_alarm_handler() == -1) {
        perror("sigaction");
        goto cleanup;
    }
    status = 0;
    for (;;) {
        long number = 0;
        int result = query_line_number(1, &number);
        if (result == 1 || (result == 0 && number == 0))
            break;
        if (result == 2) {
            if (size && fwrite(data, 1, size, stdout) != size) {
                perror("stdout");
                status = 1;
            }
            break;
        }
        if (result == 3) {
            fprintf(stderr, "Invalid line number\n");
            continue;
        }
        if (result == -1) {
            perror("input");
            status = 1;
            break;
        }
        if ((unsigned long)number > count) {
            fprintf(stderr, "No such line\n");
            continue;
        }
        struct Line line = lines[number - 1];
        if (fwrite(data + line.offset, 1, line.length, stdout) != line.length ||
            (data[line.offset + line.length - 1] != '\n' && putchar('\n') == EOF)) {
            perror("stdout");
            status = 1;
            break;
        }
    }

cleanup:
    if (data != NULL)
        munmap((void *)data, size);
    if (fd != -1)
        close(fd);
    free(lines);
    return status;
}
