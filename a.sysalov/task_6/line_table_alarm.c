#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

struct Line {
    off_t position;
    size_t length;
};
static volatile sig_atomic_t timed_out = 0;
void alarm_handler(int signal_number)
{
    (void)signal_number;
    timed_out = 1;
}

int print_file(int fd)
{
    char buffer[1024];
    ssize_t bytes_read;

    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");

        return 1;
    }

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, (size_t)bytes_read, stdout) != (size_t)bytes_read) {
            perror("fwrite");
            return 1;
        }
    }
    if (bytes_read == -1) {
        perror("read");
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *filename = "text.txt";
    int fd;
    char symbol;
    off_t position = 0;

    size_t length = 0;

    struct Line *table = NULL;

    size_t count = 0;
    int line_number;
    int status = 0;
    ssize_t bytes_read;

    struct sigaction action;

    if (argc == 2) {
        filename = argv[1];
    }

    fd = open(filename, O_RDONLY);

    if (fd == -1) {
        perror("open");

        return 1;
    }

    while ((bytes_read = read(fd, &symbol, 1)) == 1) {
        if (symbol == '\n') {
            struct Line *new_table;
            new_table = realloc(
                table,
                (count + 1) * sizeof(struct Line)
            );

            if (new_table == NULL) {
                perror("realloc");
                free(table);
                close(fd);

                return 1;
            }

            table = new_table;
            table[count].position = position - length;
            table[count].length = length;
            count++;
            length = 0;
        } else {
            length++;
        }

        position++;
    }

    if (bytes_read == -1) {
        perror("read");
        free(table);
        close(fd);
        return 1;
    }

    if (length > 0) {
        struct Line *new_table;
        new_table = realloc(
            table,
            (count + 1) * sizeof(struct Line)
        );

        if (new_table == NULL) {
            perror("realloc");
            free(table);
            close(fd);

            return 1;
        }

        table = new_table;
        table[count].position = position - length;
        table[count].length = length;
        count++;
    }

    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        free(table);
        close(fd);

        return 1;
    }

    while (1) {
        int result;
        printf("Enter line number within 5 seconds: ");
        fflush(stdout);
        timed_out = 0;
        alarm(5);
        result = scanf("%d", &line_number);
        alarm(0);

        if (timed_out) {
            printf("\nTime is over. File contents:\n");
            status = print_file(fd);
            free(table);
            close(fd);

            return status;
        }

        if (result != 1 || line_number < 0) {
            break;
        }

        if ((size_t)line_number >= count) {
            printf("There is no line with this number.\n");
            continue;
        }

        if (lseek(
                fd,
                table[line_number].position,
                SEEK_SET
            ) == -1) {
            perror("lseek");
            status = 1;
            break;
        }

        char *line = malloc(table[line_number].length + 1);

        if (line == NULL) {
            perror("malloc");
            status = 1;
            break;
        }

        size_t total = 0;

        while (total < table[line_number].length) {
            bytes_read = read(fd, line + total,
                              table[line_number].length - total);
            if (bytes_read == -1 && errno == EINTR) {
                continue;
            }
            if (bytes_read <= 0) {
                if (bytes_read == -1) {
                    perror("read");
                } else {
                    fprintf(stderr, "Unexpected end of file.\n");
                }
                status = 1;
                break;
            }
            total += (size_t)bytes_read;
        }

        if (status != 0) {
            free(line);
            break;
        }

        line[total] = '\0';
        printf("Selected line: %s\n", line);
        free(line);
    }

    free(table);
    close(fd);

    return status;
}
