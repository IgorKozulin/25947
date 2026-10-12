#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

struct line_info {
    off_t offset;
    size_t length;
};

struct line_table {
    struct line_info *items;
    size_t count;
    size_t capacity;
};

static int
append_line(struct line_table *table, off_t offset, size_t length)
{
    if (table->count == table->capacity) {
        size_t new_capacity = table->capacity == 0 ? 16 : table->capacity * 2;
        struct line_info *larger;

        if (new_capacity < table->capacity ||
            new_capacity > SIZE_MAX / sizeof(*table->items)) {
            errno = EOVERFLOW;
            return -1;
        }
        larger = realloc(table->items, new_capacity * sizeof(*table->items));
        if (larger == NULL) {
            return -1;
        }
        table->items = larger;
        table->capacity = new_capacity;
    }
    table->items[table->count].offset = offset;
    table->items[table->count].length = length;
    ++table->count;
    return 0;
}

static int
build_table(int fd, struct line_table *table)
{
    char buffer[8192];
    off_t position = 0;
    off_t line_start = 0;

    for (;;) {
        ssize_t amount = read(fd, buffer, sizeof(buffer));

        if (amount == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (amount == 0) {
            break;
        }
        for (ssize_t index = 0; index < amount; ++index, ++position) {
            if (buffer[index] == '\n') {
                off_t end = position + 1;
                if (append_line(table, line_start,
                        (size_t)(end - line_start)) == -1) {
                    return -1;
                }
                line_start = end;
            }
        }
    }
    if (position > line_start && append_line(table, line_start,
            (size_t)(position - line_start)) == -1) {
        return -1;
    }
    return 0;
}

static void
print_table(const struct line_table *table)
{
    puts("--- Debug: Line Table ---");
    for (size_t index = 0; index < table->count; ++index) {
        printf("Line %zu: Offset = %lld, Length = %zu\n", index + 1,
            (long long)table->items[index].offset,
            table->items[index].length);
    }
    puts("-------------------------");
}

/* Return 1 for a number, 0 for EOF, -1 for invalid input, -2 for I/O error. */
static int
read_line_number(uintmax_t *number)
{
    char input[128];
    size_t used = 0;
    int too_long = 0;
    char character;
    ssize_t amount;
    char *end;
    uintmax_t parsed;

    for (;;) {
        amount = read(STDIN_FILENO, &character, 1);
        if (amount == -1 && errno == EINTR) {
            continue;
        }
        if (amount == -1) {
            return -2;
        }
        if (amount == 0 || character == '\n') {
            break;
        }
        if (used + 1 < sizeof(input)) {
            input[used++] = character;
        } else {
            too_long = 1;
        }
    }
    if (amount == 0 && used == 0 && !too_long) {
        return 0;
    }
    if (too_long) {
        return -1;
    }
    input[used] = '\0';
    if (input[0] == '-') {
        return -1;
    }
    errno = 0;
    parsed = strtoumax(input, &end, 10);
    if (errno == ERANGE || end == input) {
        return -1;
    }
    while (*end != '\0' && isspace((unsigned char)*end)) {
        ++end;
    }
    if (*end != '\0') {
        return -1;
    }
    *number = parsed;
    return 1;
}

static int
print_selected_line(int fd, const struct line_info *line)
{
    char *text;
    size_t received = 0;

    if (line->length == SIZE_MAX) {
        errno = EOVERFLOW;
        return -1;
    }
    text = malloc(line->length + 1);
    if (text == NULL) {
        return -1;
    }
    if (lseek(fd, line->offset, SEEK_SET) == (off_t)-1) {
        free(text);
        return -1;
    }
    while (received < line->length) {
        ssize_t amount = read(fd, text + received, line->length - received);
        if (amount == -1 && errno == EINTR) {
            continue;
        }
        if (amount <= 0) {
            if (amount == 0) {
                errno = EIO;
            }
            free(text);
            return -1;
        }
        received += (size_t)amount;
    }
    text[line->length] = '\0';
    if (printf("%s", text) < 0 ||
        (((line->length == 0 || text[line->length - 1] != '\n') &&
            putchar('\n') == EOF))) {
        free(text);
        return -1;
    }
    free(text);
    return fflush(stdout) == EOF ? -1 : 0;
}

int
main(int argc, char **argv)
{
    struct line_table table = { NULL, 0, 0 };
    int fd;
    int status = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s text-file\n", argv[0]);
        return EXIT_FAILURE;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    if (build_table(fd, &table) == -1) {
        perror("building line table");
        status = EXIT_FAILURE;
        goto cleanup;
    }
    print_table(&table);

    for (;;) {
        uintmax_t number = 0;
        int input_status;

        printf("Enter line number (0 to quit): ");
        if (fflush(stdout) == EOF) {
            perror("fflush");
            status = EXIT_FAILURE;
            break;
        }
        input_status = read_line_number(&number);
        if (input_status == 0) {
            break;
        }
        if (input_status == -2) {
            perror("read input");
            status = EXIT_FAILURE;
            break;
        }
        if (input_status == -1) {
            puts("Please enter a non-negative integer.");
            continue;
        }
        if (number == 0) {
            break;
        }
        if (number > table.count) {
            printf("Line number is out of range (1..%zu).\n", table.count);
            continue;
        }
        if (print_selected_line(fd, &table.items[(size_t)number - 1]) == -1) {
            perror("reading selected line");
            status = EXIT_FAILURE;
            break;
        }
    }

cleanup:
    free(table.items);
    if (close(fd) == -1) {
        perror("close");
        status = EXIT_FAILURE;
    }
    return status;
}
