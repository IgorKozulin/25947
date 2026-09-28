#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

struct Line {
    off_t position;
    size_t length;
};

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

    printf("Line table:\n");

    for (size_t i = 0; i < count; i++) {
        printf("Line %zu: position = %lld, length = %zu\n",
               i,
               (long long)table[i].position,
               table[i].length);
    }

    while (1) {
        printf("\nEnter line number (negative number to exit): ");

        if (scanf("%d", &line_number) != 1) {
            break;
        }

        if (line_number < 0) {
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
