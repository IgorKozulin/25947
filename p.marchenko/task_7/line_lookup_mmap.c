#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_LINES 100000

int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: %s file\n", argv[0]); return 1; }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    struct stat st;
    if (fstat(fd, &st) != 0) { perror("fstat"); close(fd); return 1; }
    size_t size = st.st_size;
    if (size == 0) {
        printf("Empty file\n");
        close(fd);
        return 0;
    }

    char *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    off_t offsets[MAX_LINES];
    size_t lengths[MAX_LINES];
    int nlines = 0;

    size_t i;
    size_t start = 0;
    for (i = 0; i < size; ++i) {
        if (data[i] == '\n') {
            offsets[nlines] = start;
            lengths[nlines] = i - start + 1;
            nlines++;
            if (nlines >= MAX_LINES) break;
            start = i + 1;
        }
    }
    if (start < size && nlines < MAX_LINES) {
        offsets[nlines] = start;
        lengths[nlines] = size - start;
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
        fwrite(data + offsets[idx], 1, lengths[idx], stdout);
        if (lengths[idx] == 0 || data[offsets[idx] + lengths[idx] - 1] != '\n')
            printf("\n");
    }

    munmap(data, size);
    close(fd);
    return 0;
}
