#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#define MAX_LINES 100000

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

/* Глобальные — чтобы обработчик сигнала имел к ним доступ */
static char *g_filename = NULL;
static LineInfo *g_table = NULL;
static int g_nlines = 0;

/* --- Обработчик SIGALRM --- */
static void alarm_handler(int sig) {
    (void)sig;

    const char *msg = "[TIMEOUT] Time is up! Printing full file content...\n";
    write(STDOUT_FILENO, msg, strlen(msg));

    int fd = open(g_filename, O_RDONLY);
    if (fd >= 0) {
        char buf[4096];
        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, n);
        }
        close(fd);
    }
    _exit(EXIT_SUCCESS);
}

/* --- Построение таблицы строк --- */
static int build_table(int fd, LineInfo *table, int *nlines_out) {
    int nlines = 0;
    off_t pos = 0;
    off_t start = 0;
    char c;
    ssize_t r;

    while ((r = read(fd, &c, 1)) == 1) {
        pos++;
        if (c == '\n') {
            if (nlines >= MAX_LINES) break;
            table[nlines].offset = start;
            table[nlines].length = pos - start;
            nlines++;
            start = pos;
        }
    }
    if (start < pos && nlines < MAX_LINES) {
        table[nlines].offset = start;
        table[nlines].length = pos - start;
        nlines++;
    }
    *nlines_out = nlines;
    return 0;
}

/* --- Вывод одной строки --- */
static void print_line(int fd, LineInfo *li) {
    char *buf = malloc(li->length + 1);
    if (!buf) { perror("malloc"); return; }
    if (lseek(fd, li->offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        free(buf);
        return;
    }
    ssize_t got = read(fd, buf, li->length);
    if (got > 0) {
        buf[got] = '\0';
        printf("%s", buf);
        if (buf[got - 1] != '\n') printf("\n");
    }
    free(buf);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }
    g_filename = argv[1];

    int fd = open(g_filename, O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    g_table = malloc(sizeof(LineInfo) * MAX_LINES);
    if (!g_table) { perror("malloc"); close(fd); return 1; }

    build_table(fd, g_table, &g_nlines);

    /* Отладочный вывод таблицы */
    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < g_nlines; ++i) {
        printf("Line %d: Offset = %ld, Length = %zu\n",
               i + 1, (long)g_table[i].offset, g_table[i].length);
    }
    printf("-------------------------\n");

    /* Регистрируем обработчик SIGALRM */
    signal(SIGALRM, alarm_handler);

    int num;
    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);
        int rc = scanf("%d", &num);
        alarm(0);   /* ввод прошёл — гасим таймер */

        if (rc != 1) {
            /* Некорректный ввод: очистим буфер и продолжим */
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            continue;
        }
        if (num == 0) break;

        if (num < 1 || num > g_nlines) {
            printf("No such line (file has %d lines)\n", g_nlines);
            continue;
        }
        print_line(fd, &g_table[num - 1]);
    }

    close(fd);
    free(g_table);
    return 0;
}
