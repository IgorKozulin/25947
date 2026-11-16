
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>

struct LineInfo {
    size_t offset;
    size_t length;
};

static const char *file_map;
static size_t file_size;

void alarm_handler(int sig)
{
    const char message[] = "\n[TIMEOUT] Time is up! Printing full file content...\n";
    size_t written = 0;
    ssize_t n;

    (void)sig;
    write(STDOUT_FILENO, message, sizeof(message) - 1);

    while (written < file_size) {
        n = write(STDOUT_FILENO, file_map + written, file_size - written);

        if (n <= 0)
            _exit(1);

        written += (size_t)n;
    }

    _exit(0);
}

int add_line(struct LineInfo **lines, size_t *count, size_t *capacity,
             size_t offset, size_t length)
{
    if (*count == *capacity) {
        size_t new_capacity = *capacity == 0 ? 8 : *capacity * 2;
        struct LineInfo *temp = realloc(*lines, new_capacity * sizeof(**lines));

        if (temp == NULL)
            return -1;

        *lines = temp;
        *capacity = new_capacity;
    }

    (*lines)[*count].offset = offset;
    (*lines)[*count].length = length;
    (*count)++;
    return 0;
}

int main(int argc, char *argv[])
{
    int fd;
    int status = 0;
    struct stat st;
    struct LineInfo *lines = NULL;
    size_t count = 0, capacity = 0, start = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (st.st_size == 0) {
        printf("File is empty.\n");
        close(fd);
        return 0;
    }

    if (st.st_size < 0 || (off_t)(size_t)st.st_size != st.st_size) {
        fprintf(stderr, "File is too large.\n");
        close(fd);
        return 1;
    }

    file_size = (size_t)st.st_size;
    file_map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (file_map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    for (size_t i = 0; i < file_size; i++) {
        if (file_map[i] == '\n') {
            if (add_line(&lines, &count, &capacity, start, i - start + 1) == -1) {
                perror("realloc");
                status = 1;
                goto finish;
            }
            start = i + 1;
        }
    }

    if (start < file_size) {
        if (add_line(&lines, &count, &capacity, start, file_size - start) == -1) {
            perror("realloc");
            status = 1;
            goto finish;
        }
    }

    printf("--- Debug: Line Table ---\n");

    for (size_t i = 0; i < count; i++) {
        printf("Line %zu: Offset = %zu, Length = %zu\n",
               i + 1, lines[i].offset, lines[i].length);
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
        size_t index;

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

        if (errno != 0 || end == input || *end != '\0') {
            printf("Invalid input\n");
            continue;
        }

        if (number == 0)
            break;

        if (number < 1 || (unsigned long)number > count) {
            printf("Line does not exist\n");
            continue;
        }

        index = (size_t)number - 1;
        fwrite(file_map + lines[index].offset, 1, lines[index].length, stdout);

        if (file_map[lines[index].offset + lines[index].length - 1] != '\n')
            putchar('\n');
    }

finish:
    alarm(0);
    free(lines);
    if (munmap((void *)file_map, file_size) == -1) {
        perror("munmap");
        status = 1;
    }
    return status;
}

