#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MAX_LINES 100000

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    off_t offsets[MAX_LINES];
    size_t lengths[MAX_LINES];
    int nlines = 0;

    offsets[0] = 0;
    off_t pos = 0;
    char c;
    size_t cur_len = 0;
    ssize_t r;

    while ((r = read(fd, &c, 1)) == 1) {
        pos++;
        cur_len++;
        if (c == '\n') {
            lengths[nlines] = cur_len;
            nlines++;
            if (nlines >= MAX_LINES) break;
            offsets[nlines] = pos;
            cur_len = 0;
        }
    }
    /* если файл не заканчивается переводом строки — сохранить последнюю строку */
    if (cur_len > 0 && nlines < MAX_LINES) {
        lengths[nlines] = cur_len;
        nlines++;
    }

    int num;
    while (1) {
        printf("Enter line number (0 to quit): ");
        fflush(stdout);
        if (scanf("%d", &num) != 1) break;
        if (num == 0) break;
        if (num < 1 || num > nlines) {
            printf("No such line (file has %d lines)\n", nlines);
            continue;
        }
        int idx = num - 1;
        if (lseek(fd, offsets[idx], SEEK_SET) == (off_t)-1) {
            perror("lseek");
            continue;
        }
        char *line = malloc(lengths[idx] + 1);
        if (!line) { perror("malloc"); break; }
        ssize_t got = read(fd, line, lengths[idx]);
        if (got > 0) {
            line[got] = '\0';
            printf("%s", line);
            if (line[got - 1] != '\n') printf("\n");
        }
        free(line);
    }

    close(fd);
    return 0;
}
