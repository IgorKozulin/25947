#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUF_SIZE 4096

typedef struct {
    long offset;
    int  length;
} LineInfo;

typedef struct {
    LineInfo *items;
    int       count;
    int       capacity;
} LineTable;

static void table_init(LineTable *t) {
    t->count = 0;
    t->capacity = 64;
    t->items = malloc(t->capacity * sizeof(LineInfo));
    if (!t->items) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
}

static void table_push(LineTable *t, long offset, int length) {
    if (t->count == t->capacity) {
        t->capacity *= 2;
        LineInfo *tmp = realloc(t->items, t->capacity * sizeof(LineInfo));
        if (!tmp) {
            perror("realloc");
            free(t->items);
            exit(EXIT_FAILURE);
        }
        t->items = tmp;
    }
    t->items[t->count].offset = offset;
    t->items[t->count].length = length;
    t->count++;
}

static void table_free(LineTable *t) {
    free(t->items);
    t->items = NULL;
    t->count = 0;
    t->capacity = 0;
}

static void build_table(int fd, LineTable *t) {
    char buf[BUF_SIZE];
    ssize_t n;
    long line_start = 0;
    long current_pos = 0;
    int  line_len = 0;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                table_push(t, line_start, line_len);
                line_start = current_pos + i + 1;
                line_len = 0;
            } else {
                line_len++;
            }
        }
        current_pos += n;
    }

    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    if (line_len > 0 || (current_pos > 0 && line_start < current_pos)) {
        table_push(t, line_start, line_len);
    }
}

static void print_debug(const LineTable *t) {
    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < t->count; i++) {
        printf("Line %d: Offset = %ld, Length = %d\n",
               i + 1, t->items[i].offset, t->items[i].length);
    }
    printf("-------------------------\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <textfile>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    LineTable table;
    table_init(&table);
    build_table(fd, &table);
    print_debug(&table);

    int line_no;
    for (;;) {
        printf("Enter line number (0 to quit): ");
        fflush(stdout);
        if (scanf("%d", &line_no) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            printf("Invalid input, try again.\n");
            continue;
        }

        if (line_no == 0) break;

        if (line_no < 1 || line_no > table.count) {
            printf("Line %d is out of range (1..%d).\n", line_no, table.count);
            continue;
        }

        LineInfo *info = &table.items[line_no - 1];

        if (lseek(fd, info->offset, SEEK_SET) == (off_t)-1) {
            perror("lseek");
            break;
        }

        char *line = malloc(info->length + 1);
        if (!line) {
            perror("malloc");
            break;
        }

        ssize_t got = read(fd, line, info->length);
        if (got < 0) {
            perror("read");
            free(line);
            break;
        }
        line[got] = '\0';
        printf("%s\n", line);
        free(line);
    }

    close(fd);
    table_free(&table);
    return EXIT_SUCCESS;
}
