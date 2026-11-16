
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

struct LineInfo {
    off_t offset;
    size_t length;
};

static int file_fd = -1;

void alarm_handler(int sig)
{
    char buffer[4096];
    ssize_t n;
    const char message[] = "\n[TIMEOUT] Time is up! Printing full file content...\n";

    (void)sig;
    write(STDOUT_FILENO, message, sizeof(message) - 1);

    if (lseek(file_fd, 0, SEEK_SET) == (off_t)-1)
        _exit(1);

    while ((n = read(file_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t sent = 0;

        while (sent < n) {
            ssize_t w = write(STDOUT_FILENO, buffer + sent, n - sent);

            if (w <= 0)
                _exit(1);

            sent += w;
        }
    }

    _exit(n < 0 ? 1 : 0);
}

int add_line(struct LineInfo **table, size_t *count,
             size_t *capacity, off_t start, off_t end)
{
    if (*count == *capacity) {
        size_t new_capacity = *capacity == 0 ? 8 : *capacity * 2;

        struct LineInfo *temp =
            realloc(*table, new_capacity * sizeof(**table));

        if (temp == NULL)
            return -1;

        *table = temp;
        *capacity = new_capacity;
    }

    (*table)[*count].offset = start;
    (*table)[*count].length = end - start;
    (*count)++;

    return 0;
}

int main(int argc, char *argv[])
{
    struct LineInfo *table = NULL;
    size_t count = 0, capacity = 0;
    off_t start = 0, position;
    char ch;
    ssize_t n;
    int status = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    file_fd = open(argv[1], O_RDONLY);

    if (file_fd == -1) {
        perror("open");
        return 1;
    }

    while ((n = read(file_fd, &ch, 1)) == 1) {
        if (ch == '\n') {
            position = lseek(file_fd, 0, SEEK_CUR);

            if (position == (off_t)-1 ||
                add_line(&table, &count, &capacity, start, position) == -1) {
                perror("index");
                status = 1;
                goto finish;
            }

            start = position;
        }
    }

    if (n == -1) {
        perror("read");
        status = 1;
        goto finish;
    }

    position = lseek(file_fd, 0, SEEK_CUR);

    if (position == (off_t)-1) {
        perror("lseek");
        status = 1;
        goto finish;
    }

    if (position > start &&
        add_line(&table, &count, &capacity, start, position) == -1) {
        perror("realloc");
        status = 1;
        goto finish;
    }

    printf("--- Debug: Line Table ---\n");

    for (size_t i = 0; i < count; i++) {
        printf("Line %zu: Offset = %lld, Length = %zu\n",
               i + 1, (long long)table[i].offset, table[i].length);
    }

    printf("-------------------------\n");

    if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
        perror("signal");
        status = 1;
        goto finish;
    }

    while (1) {
        char input[128], *end;
        long number;

        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);
        char *result = fgets(input, sizeof(input), stdin);
        alarm(0);

        if (result == NULL)
            break;

        errno = 0;
        number = strtol(input, &end, 10);

        while (isspace((unsigned char)*end))
            end++;

        if (end == input || *end != '\0' || errno != 0) {
            printf("Invalid input\n");
            continue;
        }

        if (number == 0)
            break;

        if (number < 0 || (unsigned long)number > count) {
            printf("Line does not exist\n");
            continue;
        }

        struct LineInfo info = table[number - 1];
        char *line = malloc(info.length + 1);

        if (line == NULL) {
            perror("malloc");
            status = 1;
            break;
        }

        if (lseek(file_fd, info.offset, SEEK_SET) == (off_t)-1) {
            perror("lseek");
            free(line);
            status = 1;
            break;
        }

        size_t total = 0;

        while (total < info.length) {
            n = read(file_fd, line + total, info.length - total);

            if (n <= 0) {
                if (n < 0)
                    perror("read");
                else
                    fprintf(stderr, "Unexpected end of file\n");

                status = 1;
                break;
            }

            total += n;
        }

        if (status != 0) {
            free(line);
            break;
        }

        line[info.length] = '\0';

        printf("%s", line);

        if (info.length == 0 || line[info.length - 1] != '\n')
            putchar('\n');

        free(line);
    }

finish:
    alarm(0);
    free(table);
    close(file_fd);

    return status;
}

