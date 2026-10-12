
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_LINES 1000

typedef struct {
    int number;
    size_t offset;
    size_t length;
} LineInfo;

volatile sig_atomic_t timed_out = 0;

void alarm_handler(int signal_number)
{
    (void)signal_number;
    timed_out = 1;
}

/* Ввод номера строки с ограничением по времени */
int read_line_number(int *number)
{
    char input[64];
    size_t pos = 0;
    char ch;

    timed_out = 0;
    alarm(5);

    while (pos < sizeof(input) - 1) {
        ssize_t n = read(STDIN_FILENO, &ch, 1);

        if (n == -1 && errno == EINTR) {
            if (timed_out)
                return 0;
            continue;
        }

        if (n == -1) {
            alarm(0);
            perror("read");
            return -1;
        }

        if (n == 0)
            break;

        if (ch == '\n')
            break;

        input[pos++] = ch;
    }

    alarm(0);

    if (timed_out)
        return 0;

    input[pos] = '\0';

    char *end;
    long value = strtol(input, &end, 10);

    while (*end == ' ' || *end == '\t' || *end == '\r')
        end++;

    if (end == input || *end != '\0' ||
        value < 0 || value > 2147483647L) {
        printf("Некорректный номер строки.\n");
        return -1;
    }

    *number = (int)value;
    return 1;
}

int main(void)
{
    int fd = open("text.txt", O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct stat st;

    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (st.st_size == 0) {
        printf("Файл пуст.\n");
        close(fd);
        return 0;
    }

    size_t file_size = (size_t)st.st_size;

    char *mapping = mmap(NULL, file_size, PROT_READ,
                         MAP_PRIVATE, fd, 0);

    if (mapping == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    close(fd);

    LineInfo lines[MAX_LINES];
    int line_count = 0;
    size_t start = 0;

    /* Построение таблицы строк по отображённой памяти */
    for (size_t i = 0; i < file_size; i++) {
        if (mapping[i] == '\n') {
            if (line_count >= MAX_LINES) {
                fprintf(stderr, "Слишком много строк в файле.\n");
                munmap(mapping, file_size);
                return 1;
            }

            lines[line_count].number = line_count + 1;
            lines[line_count].offset = start;
            lines[line_count].length = i - start + 1;
            line_count++;
            start = i + 1;
        }
    }

    /* Последняя строка может не заканчиваться символом новой строки */
    if (start < file_size) {
        if (line_count >= MAX_LINES) {
            fprintf(stderr, "Слишком много строк в файле.\n");
            munmap(mapping, file_size);
            return 1;
        }

        lines[line_count].number = line_count + 1;
        lines[line_count].offset = start;
        lines[line_count].length = file_size - start;
        line_count++;
    }

    printf("Number\tOffset\tLength\n");

    for (int i = 0; i < line_count; i++) {
        printf("%d\t%zu\t%zu\n",
               lines[i].number,
               lines[i].offset,
               lines[i].length);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        munmap(mapping, file_size);
        return 1;
    }

    while (1) {
        int number = 0;

        printf("\nВведите номер строки (0 — выход): ");
        fflush(stdout);

        int result = read_line_number(&number);

        if (result == 0) {
            printf("\nВремя истекло! Содержимое файла:\n\n");
            fwrite(mapping, 1, file_size, stdout);
            break;
        }

        if (result == -1)
            continue;

        if (number == 0)
            break;

        if (number < 1 || number > line_count) {
            printf("Такой строки нет.\n");
            continue;
        }

        fwrite(mapping + lines[number - 1].offset,
               1, lines[number - 1].length, stdout);
    }

    alarm(0);
    munmap(mapping, file_size);

    return 0;
}

