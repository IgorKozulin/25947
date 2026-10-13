#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

char *g_filename = NULL;
int g_fd = -1;

void alarm_handler(int sig) {
    (void)sig;
    char buf[4096];
    ssize_t n;
    int fd = open(g_filename, O_RDONLY);
    if (fd < 0) _exit(EXIT_FAILURE);
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, n);
    }
    close(fd);
    _exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    g_filename = argv[1];
    g_fd = open(g_filename, O_RDONLY);
    if (g_fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    LineInfo *table = NULL;
    size_t capacity = 16, count = 0;
    table = malloc(capacity * sizeof(LineInfo));

    off_t line_start = 0;
    off_t pos = 0;
    char ch;
    ssize_t n;

    while ((n = read(g_fd, &ch, 1)) == 1) {
        if (ch == '\n') {
            if (count == capacity) {
                capacity *= 2;
                table = realloc(table, capacity * sizeof(LineInfo));
            }
            table[count].offset = line_start;
            table[count].length = pos - line_start + 1;
            count++;
            line_start = pos + 1;
        }
        pos++;
    }
    if (pos > line_start) {
        if (count == capacity) {
            capacity *= 2;
            table = realloc(table, capacity * sizeof(LineInfo));
        }
        table[count].offset = line_start;
        table[count].length = pos - line_start;
        count++;
    }

    printf("--- Debug: Line Table ---\n");
    for (size_t i = 0; i < count; i++) {
        printf("Line %zu: Offset = %ld, Length = %zu\n",
               i + 1, (long)table[i].offset, table[i].length);
    }
    printf("-------------------------\n");

    signal(SIGALRM, alarm_handler);

    char linebuf[4096];
    int num;

    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);

        if (scanf("%d", &num) != 1) {
            alarm(0);
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        alarm(0);

        if (num == 0) break;

        if (num < 1 || (size_t)num > count) {
            printf("Invalid line number. Valid range: 1..%zu\n", count);
            continue;
        }

        size_t len = table[num - 1].length;
        if (len >= sizeof(linebuf)) len = sizeof(linebuf) - 1;

        lseek(g_fd, table[num - 1].offset, SEEK_SET);
        ssize_t r = read(g_fd, linebuf, len);
        if (r > 0) {
            linebuf[r] = '\0';
            printf("%s", linebuf);
            if (linebuf[r - 1] != '\n') printf("\n");
        }
    }

    close(g_fd);
    free(table);
    return EXIT_SUCCESS;
}
