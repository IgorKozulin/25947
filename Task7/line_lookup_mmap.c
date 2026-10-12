#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct line_info {
    size_t offset;
    size_t length;
};

struct line_table {
    struct line_info *items;
    size_t count;
    size_t capacity;
};

static const char *mapped_file;
static size_t mapped_size;

static int
append_line(struct line_table *table, size_t offset, size_t length)
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
build_table(const char *data, size_t size, struct line_table *table)
{
    size_t line_start = 0;

    for (size_t position = 0; position < size; ++position) {
        if (data[position] == '\n') {
            size_t end = position + 1;
            if (append_line(table, line_start, end - line_start) == -1) {
                return -1;
            }
            line_start = end;
        }
    }
    if (line_start < size &&
        append_line(table, line_start, size - line_start) == -1) {
        return -1;
    }
    return 0;
}

static void
print_table(const struct line_table *table)
{
    puts("--- Debug: Line Table ---");
    for (size_t index = 0; index < table->count; ++index) {
        printf("Line %zu: Offset = %zu, Length = %zu\n", index + 1,
            table->items[index].offset, table->items[index].length);
    }
    puts("-------------------------");
}

static int
write_all(int fd, const char *buffer, size_t length)
{
    size_t written = 0;

    while (written < length) {
        ssize_t amount = write(fd, buffer + written, length - written);
        if (amount == -1 && errno == EINTR) {
            continue;
        }
        if (amount <= 0) {
            return -1;
        }
        written += (size_t)amount;
    }
    return 0;
}

static void
alarm_handler(int signal_number)
{
    static const char message[] =
        "\n[TIMEOUT] Time is up! Printing full file content...\n";
    (void)signal_number;

    (void)write_all(STDOUT_FILENO, message, sizeof(message) - 1);
    (void)write_all(STDOUT_FILENO, mapped_file, mapped_size);
    _exit(EXIT_SUCCESS);
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

int
main(int argc, char **argv)
{
    struct line_table table = { NULL, 0, 0 };
    struct stat info;
    struct sigaction action;
    int fd = -1;
    int status = EXIT_SUCCESS;
    void *mapping = MAP_FAILED;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s text-file\n", argv[0]);
        return EXIT_FAILURE;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    if (fstat(fd, &info) == -1) {
        perror("fstat");
        status = EXIT_FAILURE;
        goto cleanup;
    }
    if (info.st_size < 0 || (uintmax_t)info.st_size > SIZE_MAX) {
        fprintf(stderr, "File is too large to map.\n");
        status = EXIT_FAILURE;
        goto cleanup;
    }
    mapped_size = (size_t)info.st_size;
    if (mapped_size == 0) {
        puts("File is empty.");
        goto cleanup;
    }
    mapping = mmap(NULL, mapped_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapping == MAP_FAILED) {
        perror("mmap");
        status = EXIT_FAILURE;
        goto cleanup;
    }
    mapped_file = mapping;
    if (close(fd) == -1) {
        perror("close");
        fd = -1;
        status = EXIT_FAILURE;
        goto cleanup;
    }
    fd = -1;
    if (build_table(mapped_file, mapped_size, &table) == -1) {
        perror("building line table");
        status = EXIT_FAILURE;
        goto cleanup;
    }
    print_table(&table);

    memset(&action, 0, sizeof(action));
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction(SIGALRM)");
        status = EXIT_FAILURE;
        goto cleanup;
    }

    for (;;) {
        uintmax_t number = 0;
        int input_status;

        printf("Enter line number (0 to quit, 5 sec timeout): ");
        if (fflush(stdout) == EOF) {
            perror("fflush");
            status = EXIT_FAILURE;
            break;
        }
        (void)alarm(5);
        input_status = read_line_number(&number);
        (void)alarm(0);
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
        {
            const struct line_info *line = &table.items[(size_t)number - 1];
            if (fwrite(mapped_file + line->offset, 1, line->length, stdout) !=
                    line->length) {
                perror("fwrite");
                status = EXIT_FAILURE;
                break;
            }
            if ((line->length == 0 ||
                    mapped_file[line->offset + line->length - 1] != '\n') &&
                putchar('\n') == EOF) {
                perror("putchar");
                status = EXIT_FAILURE;
                break;
            }
            if (fflush(stdout) == EOF) {
                perror("fflush");
                status = EXIT_FAILURE;
                break;
            }
        }
    }

cleanup:
    free(table.items);
    if (mapping != MAP_FAILED && munmap(mapping, mapped_size) == -1) {
        perror("munmap");
        status = EXIT_FAILURE;
    }
    if (fd != -1 && close(fd) == -1) {
        perror("close");
        status = EXIT_FAILURE;
    }
    return status;
}
