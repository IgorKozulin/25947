#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

static char *global_map = NULL;
static size_t global_size = 0;

void alarm_handler(int sig) {
    (void)sig;
    const char *msg = "\n[TIMEOUT] Time is up! Printing full file content...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    if (global_map && global_size > 0) {
        write(STDOUT_FILENO, global_map, global_size);
    }
    _exit(EXIT_SUCCESS);
}

typedef struct {
    size_t offset;
    size_t length;
} LineInfo;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    if (st.st_size == 0) {
        printf("File is empty.\n");
        close(fd);
        return EXIT_SUCCESS;
    }

    global_size = (size_t)st.st_size;

    global_map = mmap(NULL, global_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (global_map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    LineInfo *lines = NULL;
    size_t line_count = 0;
    size_t capacity = 16;
    lines = malloc(capacity * sizeof(LineInfo));
    if (!lines) {
        perror("malloc");
        munmap(global_map, global_size);
        return EXIT_FAILURE;
    }

    size_t start = 0;
    for (size_t i = 0; i < global_size; i++) {
        if (global_map[i] == '\n') {
            if (line_count >= capacity) {
                capacity *= 2;
                LineInfo *tmp = realloc(lines, capacity * sizeof(LineInfo));
                if (!tmp) {
                    perror("realloc");
                    free(lines);
                    munmap(global_map, global_size);
                    return EXIT_FAILURE;
                }
                lines = tmp;
            }
            lines[line_count].offset = start;
            lines[line_count].length = i - start;
            line_count++;
            start = i + 1;
        }
    }
    if (start < global_size) {
        if (line_count >= capacity) {
            capacity++;
            LineInfo *tmp = realloc(lines, capacity * sizeof(LineInfo));
            if (!tmp) {
                perror("realloc");
                free(lines);
                munmap(global_map, global_size);
                return EXIT_FAILURE;
            }
            lines = tmp;
        }
        lines[line_count].offset = start;
        lines[line_count].length = global_size - start;
        line_count++;
    }

    printf("--- Debug: Line Table ---\n");
    for (size_t i = 0; i < line_count; i++) {
        printf("Line %zu: Offset = %zu, Length = %zu\n",
               i + 1, lines[i].offset, lines[i].length);
    }
    printf("-------------------------\n");

    signal(SIGALRM, alarm_handler);

    char input[256];
    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            alarm(0);
            break;
        }

        alarm(0);

        input[strcspn(input, "\n")] = '\0';

        char *endptr;
        long num = strtol(input, &endptr, 10);
        if (*endptr != '\0' || endptr == input) {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        if (num == 0) {
            break;
        }

        if (num < 1 || (size_t)num > line_count) {
            printf("Line number out of range (1-%zu).\n", line_count);
            continue;
        }

        LineInfo *li = &lines[num - 1];
        fwrite(global_map + li->offset, 1, li->length, stdout);
        printf("\n");
    }

    free(lines);
    munmap(global_map, global_size);

    return EXIT_SUCCESS;
}
