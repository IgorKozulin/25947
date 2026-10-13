
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1024

typedef struct {
    int number;
    off_t offset;
    size_t length;
} LineInfo;

volatile sig_atomic_t timed_out = 0;

void alarm_handler(int signal_number)
{
    (void)signal_number;
    timed_out = 1;
}

/* Печать всего содержимого файла */
void print_entire_file(int fd)
{
    char buffer[1024];
    ssize_t bytes_read;

    printf("\nВремя истекло! Содержимое файла:\n\n");

    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        return;
    }

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        ssize_t written = 0;

        while (written < bytes_read) {
            ssize_t n = write(STDOUT_FILENO,
                              buffer + written,
                              (size_t)(bytes_read - written));

            if (n == -1) {
                if (errno == EINTR)
                    continue;
                perror("write");
                return;
            }

            written += n;
        }
    }

    if (bytes_read == -1)
        perror("read");
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

    if (end == input || *end != '\0') {
        printf("Некорректный номер строки.\n");
        return -1;
    }

    if (value < 0 || value > 2147483647L) {
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

    LineInfo lines[MAX_LINES];
    int line_count = 0;
    off_t offset = 0;
    size_t length = 0;
    char ch;

    /* Построение таблицы смещений и длин строк */
    while (read(fd, &ch, 1) == 1) {
        if (line_count >= MAX_LINES) {
            fprintf(stderr, "Слишком много строк в файле.\n");
            close(fd);
            return 1;
        }

        if (length == 0) {
            lines[line_count].number = line_count + 1;
            lines[line_count].offset = offset;
        }

        offset++;
        length++;

        if (ch == '\n') {
            lines[line_count].length = length;
            line_count++;
            length = 0;
        }
    }

    /* Учёт последней строки без символа новой строки */
    if (length > 0) {
        if (line_count >= MAX_LINES) {
            fprintf(stderr, "Слишком много строк в файле.\n");
            close(fd);
            return 1;
        }

        lines[line_count].number = line_count + 1;
        lines[line_count].offset = offset - (off_t)length;
        lines[line_count].length = length;
        line_count++;
    }

    printf("Number\tOffset\tLength\n");

    for (int i = 0; i < line_count; i++) {
        printf("%d\t%lld\t%zu\n",
               lines[i].number,
               (long long)lines[i].offset,
               lines[i].length);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        close(fd);
        return 1;
    }

    while (1) {
        int number = 0;

        printf("\nВведите номер строки (0 — выход): ");
        fflush(stdout);

        int result = read_line_number(&number);

        if (result == 0) {
            print_entire_file(fd);
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

        if (lseek(fd, lines[number - 1].offset, SEEK_SET) == -1) {
            perror("lseek");
            break;
        }

        char buffer[MAX_LINE_LENGTH];
        size_t remaining = lines[number - 1].length;

        while (remaining > 0) {
            size_t to_read = remaining < sizeof(buffer)
                           ? remaining : sizeof(buffer);

            ssize_t n = read(fd, buffer, to_read);

            if (n <= 0) {
                if (n == -1)
                    perror("read");
                break;
            }

            if (write(STDOUT_FILENO, buffer, (size_t)n) == -1) {
                perror("write");
                break;
            }

            remaining -= (size_t)n;
        }
    }

    alarm(0);
    close(fd);
    return 0;
}

