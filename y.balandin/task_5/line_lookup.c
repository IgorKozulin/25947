#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct {
    long offset;   /* позиция начала строки в файле */
    int  length;   /* длина строки без '\n' */
} LineInfo;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /* --- 1. Построение таблицы строк --- */
    LineInfo *table = NULL;
    int num_lines = 0;
    int capacity  = 0;

    char c;
    long pos = 0;          /* текущая позиция в файле */
    long line_start = 0;   /* offset начала текущей строки */
    int  line_len = 0;     /* длина текущей строки */

    while (read(fd, &c, 1) == 1) {
        pos++;
        if (c == '\n') {
            /* конец строки — записываем в таблицу */
            if (num_lines == capacity) {
                capacity = capacity ? capacity * 2 : 16;
                table = realloc(table, capacity * sizeof(LineInfo));
            }
            table[num_lines].offset = line_start;
            table[num_lines].length = line_len;
            num_lines++;

            line_start = pos;   /* следующая строка начинается после '\n' */
            line_len = 0;
        } else {
            line_len++;
        }
    }

    /* последняя строка без '\n' в конце файла */
    if (line_len > 0) {
        if (num_lines == capacity) {
            capacity = capacity ? capacity * 2 : 16;
            table = realloc(table, capacity * sizeof(LineInfo));
        }
        table[num_lines].offset = line_start;
        table[num_lines].length = line_len;
        num_lines++;
    }

    /* --- 2. Отладочный вывод таблицы --- */
    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < num_lines; i++) {
        printf("Line %d: Offset = %ld, Length = %d\n",
               i + 1, table[i].offset, table[i].length);
    }
    printf("-------------------------\n");

    /* --- 3. Интерактивный запрос --- */
    int num;
    while (1) {
        printf("Enter line number (0 to quit): ");
        if (scanf("%d", &num) != 1) break;

        if (num == 0) break;

        if (num < 1 || num > num_lines) {
            printf("No such line (1..%d)\n", num_lines);
            continue;
        }

        LineInfo li = table[num - 1];

        /* переходим к началу строки */
        if (lseek(fd, li.offset, SEEK_SET) == -1) {
            perror("lseek");
            continue;
        }

        /* читаем ровно length байт */
        char *buf = malloc(li.length + 1);
        int n = read(fd, buf, li.length);
        if (n < 0) {
            perror("read");
            free(buf);
            continue;
        }
        buf[n] = '\0';

        printf("%s\n", buf);
        free(buf);
    }

    /* --- 4. Очистка --- */
    close(fd);
    free(table);
    return 0;
}
