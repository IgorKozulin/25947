#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

typedef struct {
    long offset;
    int  length;
} LineInfo;

static int   g_fd;
static char *g_filename;

/* Обработчик таймаута: печатаем весь файл и выходим */
void on_alarm(int sig)
{
    (void)sig;
    const char *msg = "\n[TIMEOUT] Printing full file:\n";
    write(1, msg, strlen(msg));

    int fd = open(g_filename, O_RDONLY);
    char buf[4096];
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, n);
    close(fd);
    _exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "Usage: %s <file>\n", argv[0]); return 1; }

    g_filename = argv[1];
    g_fd = open(g_filename, O_RDONLY);
    if (g_fd == -1) { perror("open"); return 1; }

    /* --- строим таблицу строк --- */
    LineInfo *table = NULL;
    int count = 0, cap = 0;
    char c;
    long pos = 0, start = 0;
    int len = 0;

    while (read(g_fd, &c, 1) == 1) {
        pos++;
        if (c == '\n') {
            if (count == cap) { cap = cap ? cap * 2 : 16; table = realloc(table, cap * sizeof(LineInfo)); }
            table[count].offset = start;
            table[count].length = len;
            count++;
            start = pos;
            len = 0;
        } else len++;
    }
    if (len > 0) {   /* последняя строка без '\n' */
        if (count == cap) { cap = cap ? cap * 2 : 16; table = realloc(table, cap * sizeof(LineInfo)); }
        table[count].offset = start;
        table[count].length = len;
        count++;
    }

    /* --- отладка --- */
    printf("--- Line Table ---\n");
    for (int i = 0; i < count; i++)
        printf("Line %d: Offset = %ld, Length = %d\n", i + 1, table[i].offset, table[i].length);
    printf("------------------\n");

    /* --- устанавливаем обработчик --- */
    signal(SIGALRM, on_alarm);

    /* --- цикл запросов --- */
    int num;
    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);
        alarm(5);

        if (scanf("%d", &num) != 1) {
            alarm(0);
            while (getchar() != '\n');
            continue;
        }
        alarm(0);

        if (num == 0) break;
        if (num < 1 || num > count) { printf("No such line\n"); continue; }

        LineInfo li = table[num - 1];
        lseek(g_fd, li.offset, SEEK_SET);

        char *buf = malloc(li.length + 1);
        int n = read(g_fd, buf, li.length);
        buf[n] = '\0';
        printf("%s\n", buf);
        free(buf);
    }

    close(g_fd);
    free(table);
    return 0;
}
