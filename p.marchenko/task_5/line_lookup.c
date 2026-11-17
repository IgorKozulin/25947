#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct {
    long offset;   /* позиция начала строки в файле */
    int  length;   /* длина строки, включая '\n' */
} LineInfo;

/* Читаем файл побайтно, строим таблицу, растем через realloc */
static LineInfo *build_table(int fd, int *nlines_out) {
    int capacity = 16;
    int nlines = 0;
    LineInfo *table = malloc(sizeof(LineInfo) * capacity);
    if (!table) { perror("malloc"); exit(1); }

    char c;
    long pos = 0;          /* текущая позиция в файле */
    long line_start = 0;   /* начало текущей строки */

    while (read(fd, &c, 1) == 1) {
        pos++;
        if (c == '\n') {
            if (nlines == capacity) {
                capacity *= 2;
                LineInfo *tmp = realloc(table, sizeof(LineInfo) * capacity);
                if (!tmp) { perror("realloc"); free(table); exit(1); }
                table = tmp;
            }
            table[nlines].offset = line_start;
            table[nlines].length = (int)(pos - line_start);
            nlines++;
            line_start = pos;
        }
    }
    /* последняя строка без '\n' */
    if (line_start < pos) {
        if (nlines == capacity) {
            capacity += 1;
            LineInfo *tmp = realloc(table, sizeof(LineInfo) * capacity);
            if (!tmp) { perror("realloc"); free(table); exit(1); }
            table = tmp;
        }
        table[nlines].offset = line_start;
        table[nlines].length = (int)(pos - line_start);
        nlines++;
    }
    *nlines_out = nlines;
    return table;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    int nlines = 0;
    LineInfo *table = build_table(fd, &nlines);

    /* Отладочный вывод таблицы */
    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < nlines; ++i) {
        printf("Line %d: Offset = %ld, Length = %d\n",
               i + 1, table[i].offset, table[i].length);
    }
    printf("-------------------------\n");

    int num;
    while (1) {
        printf("Enter line number (0 to quit): ");
        fflush(stdout);
        if (scanf("%d", &num) != 1) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            continue;
        }
        if (num == 0) break;
        if (num < 1 || num > nlines) {
            printf("No such line (file has %d lines)\n", nlines);
            continue;
        }
        LineInfo *li = &table[num - 1];
        if (lseek(fd, li->offset, SEEK_SET) == (off_t)-1) {
            perror("lseek");
            continue;
        }
        char *buf = malloc(li->length + 1);
        if (!buf) { perror("malloc"); break; }
        ssize_t got = read(fd, buf, li->length);
        if (got > 0) {
            buf[got] = '\0';
            printf("%s", buf);
            if (buf[got - 1] != '\n') printf("\n");
        }
        free(buf);
    }

    free(table);
    close(fd);
    return 0;
}
