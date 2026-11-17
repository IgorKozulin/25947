#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_LINES 100000

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

static char *g_map  = NULL;
static size_t g_size = 0;

static void alarm_handler(int sig) {
    (void)sig;
    const char *msg = "[TIMEOUT] Time is up! Printing full file content...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    if (g_map && g_size > 0)
        write(STDOUT_FILENO, g_map, g_size);
    _exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: %s file\n", argv[0]); return 1; }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    struct stat st;
    if (fstat(fd, &st) != 0) { perror("fstat"); close(fd); return 1; }
    g_size = st.st_size;
    if (g_size == 0) { printf("File is empty.\n"); close(fd); return 0; }

    g_map = mmap(NULL, g_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (g_map == MAP_FAILED) { perror("mmap"); close(fd); return 1; }
    close(fd);

    LineInfo *table = malloc(sizeof(LineInfo) * MAX_LINES);
    if (!table) { perror("malloc"); munmap(g_map, g_size); return 1; }

    int nlines = 0;
    size_t i, start = 0;
    for (i = 0; i < g_size; ++i) {
        if (g_map[i] == '\n') {
            if (nlines >= MAX_LINES) break;
            table[nlines].offset = start;
            table[nlines].length = i - start + 1;
            nlines++;
            start = i + 1;
        }
    }
    if (start < g_size && nlines < MAX_LINES) {
        table[nlines].offset = start;
        table[nlines].length = g_size - start;
        nlines++;
    }

    printf("--- Debug: Line Table ---\n");
    for (int k = 0; k < nlines; ++k)
        printf("Line %d: Offset = %ld, Length = %zu\n",
               k + 1, (long)table[k].offset, table[k].length);
    printf("-------------------------\n");

    signal(SIGALRM, alarm_handler);

    int num;
    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);
        int rc = scanf("%d", &num);
        alarm(0);

        if (rc != 1) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            continue;
        }
        if (num == 0) break;
        if (num < 1 || num > nlines) {
            printf("No such line (file has %d lines)\n", nlines);
            continue;
        }
        int idx = num - 1;
        fwrite(g_map + table[idx].offset, 1, table[idx].length, stdout);
        if (table[idx].length == 0 ||
            g_map[table[idx].offset + table[idx].length - 1] != '\n')
            printf("\n");
    }

    free(table);
    munmap(g_map, g_size);
    return 0;
}
