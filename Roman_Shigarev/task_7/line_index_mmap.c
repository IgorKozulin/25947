#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

typedef struct {
    size_t offset;
    size_t length;
} Line;

static volatile sig_atomic_t timed_out = 0;

static void on_alarm(int signo) {
    int saved_errno = errno;
    (void)signo;
    timed_out = 1;
    close(STDIN_FILENO);
    errno = saved_errno;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    Line *lines = NULL;
    char *data = MAP_FAILED;
    size_t size = 0, count = 0, capacity = 0, start = 0;
    int status = 1;
    struct stat info;

    if (fstat(fd, &info) == -1) {
        perror("fstat");
        goto done;
    }

    if (!S_ISREG(info.st_mode)) {
        fputs("Not a regular file\n", stderr);
        goto done;
    }

    if (info.st_size < 0 || (uintmax_t)info.st_size > SIZE_MAX) {
        fputs("File too large\n", stderr);
        goto done;
    }

    size = (size_t)info.st_size;

    if (size == 0) {
        puts("File is empty.");
        status = 0;
        goto done;
    }

    data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        goto done;
    }

    for (size_t i = 0; i < size; i++) {
        if (data[i] != '\n' && i + 1 != size) continue;

        if (count == capacity) {
            size_t next = capacity == 0 ? 16 : capacity * 2;

            if (next < capacity || next > (size_t)-1 / sizeof(*lines)) {
                fputs("Too many lines\n", stderr);
                goto done;
            }

            Line *temp = realloc(lines, next * sizeof(*lines));
            if (temp == NULL) {
                perror("realloc");
                goto done;
            }

            lines = temp;
            capacity = next;
        }

        lines[count].offset = start;
        lines[count].length = i - start + 1;
        count++;
        start = i + 1;
    }

    puts("Line Offset Length");

    for (size_t i = 0; i < count; i++) {
        printf("%zu %zu %zu\n", i + 1,
               lines[i].offset, lines[i].length);
    }

    struct sigaction action = {0};
    action.sa_handler = on_alarm;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        goto done;
    }

    char input[128];

    for (;;) {
        printf("Line number (0 to exit): ");
        fflush(stdout);

        alarm(5);
        char *received = fgets(input, sizeof(input), stdin);
        int too_long = 0;

        if (received != NULL) {
            size_t used = 0;
            while (input[used] != '\0' && input[used] != '\n') used++;

            if (input[used] != '\n' && !feof(stdin)) {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                too_long = 1;
            }
        }

        alarm(0);

        if (timed_out) {
            puts("\nTime is up. Full file:");

            if (fwrite(data, 1, size, stdout) != size) {
                perror("fwrite");
                goto done;
            }

            putchar('\n');
            status = 0;
            goto done;
        }

        if (received == NULL) {
            if (ferror(stdin)) {
                perror("fgets");
                goto done;
            }
            break;
        }

        if (too_long) {
            puts("Input too long");
            continue;
        }

        char *end;
        errno = 0;
        long number = strtol(input, &end, 10);

        if (end == input || errno == ERANGE) {
            puts("Invalid number");
            continue;
        }

        while (isspace((unsigned char)*end)) end++;

        if (*end != '\0' || number < 0 ||
            (unsigned long)number > count) {
            puts("Invalid line number");
            continue;
        }

        if (number == 0) break;

        Line selected = lines[number - 1];

        if (fwrite(data + selected.offset, 1,
                   selected.length, stdout) != selected.length) {
            perror("fwrite");
            goto done;
        }

        if (data[selected.offset + selected.length - 1] != '\n') {
            putchar('\n');
        }
    }

    status = 0;

done:
    free(lines);

    if (data != MAP_FAILED && munmap(data, size) == -1) {
        perror("munmap");
        status = 1;
    }

    if (close(fd) == -1) {
        perror("close");
        status = 1;
    }

    return status;
}
