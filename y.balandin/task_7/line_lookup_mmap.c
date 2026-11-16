#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct {
    long offset;
    int  length;
} LineInfo;

/* Глобальные — нужны обработчику сигнала */
static char *g_map  = NULL;
static long  g_size = 0;

/* Обработчик таймаута: пишем всё содержимое файла и выходим */
void on_alarm(int sig)
{
    (void)sig;
    const char *msg = "\n[TIMEOUT] Time is up! Printing full file content...\n";
    write(1, msg, strlen(msg));
    write(1, g_map, g_size);
    _exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "Usage: %s <file>\n", argv[0]); return 1; }

    /* --- 1. Открываем файл и узнаём размер --- */
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) { perror("open"); return 1; }

    struct stat st;
    fstat(fd, &st);

    if (st.st_size == 0) {
        printf("File is empty.\n");
        close(fd);
        return 0;
    }

    g_size = st.st_size;

    /* --- 2. Отображаем файл в память --- */
    g_map = mmap(NULL, g_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (g_map == MAP_FAILED) { perror("mmap"); close(fd); return 1; }
    close(fd);   /* дескриптор больше не нужен */

    /* --- 3. Строим таблицу строк --- */
    LineInfo *table = NULL;
    int count = 0, cap = 0;
    long start = 0;

    for (long i = 0; i < g_size; i++) {
        if (g_map[i] == '\n') {
            if (count == cap) { cap = cap ? cap * 2 : 16; table = realloc(table, cap * sizeof(LineInfo)); }
            table[count].offset = start;
            table[count].length = i - start;
            count++;
            start = i + 1;
        }
    }
    if (start < g_size) {   /* последняя строка без '\n' */
        if (count == cap) { cap = cap ? cap * 2 : 16; table = realloc(table, cap * sizeof(LineInfo)); }
        table[count].offset = start;
        table[count].length = g_size - start;
        count++;
    }

    /* --- 4. Отладка --- */
    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < count; i++)
        printf("Line %d: Offset = %ld, Length = %d\n", i + 1, table[i].offset, table[i].length);
    printf("-------------------------\n");

    /* --- 5. Обработчик таймаута --- */
    signal(SIGALRM, on_alarm);

    /* --- 6. Цикл запросов --- */
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

        /* вывод строки прямо из отображённой памяти */
        LineInfo li = table[num - 1];
        fwrite(g_map + li.offset, 1, li.length, stdout);
        putchar('\n');
    }

    /* --- 7. Очистка --- */
    munmap(g_map, g_size);
    free(table);
    return 0;
}
