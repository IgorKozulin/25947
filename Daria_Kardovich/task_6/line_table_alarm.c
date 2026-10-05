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

void print_file(int fd)
{
    char buffer[1024];
    ssize_t bytes_read;

    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        return;
    }

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, (size_t)bytes_read, stdout);
    }
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
    struct sigaction action;

    if (argc == 2) {
        filename = argv[1];
    }

    fd = open(filename, O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    while (read(fd, &symbol, 1) == 1) {
        if (symbol == '\n') {
            struct Line *new_table;

            new_table = realloc(table, (count + 1) * sizeof(struct Line));

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

    if (length > 0) {
        struct Line *new_table;

        new_table = realloc(table, (count + 1) * sizeof(struct Line));

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
            print_file(fd);
            free(table);
            close(fd);
            return 0;
        }

        if (result != 1 || line_number < 0) {
            break;
        }

        if ((size_t)line_number >= count) {
            printf("There is no line with this number.\n");
            continue;
        }

        if (lseek(fd, table[line_number].position, SEEK_SET) == -1) {
            perror("lseek");
            break;
        }

        char *line = malloc(table[line_number].length + 1);

        if (line == NULL) {
            perror("malloc");
            break;
        }

        if (read(fd, line, table[line_number].length) == -1) {
            perror("read");
            free(line);
            break;
        }

        line[table[line_number].length] = '\0';

        printf("Selected line: %s\n", line);

        free(line);
    }

    free(table);
    close(fd);

    return 0;
}
