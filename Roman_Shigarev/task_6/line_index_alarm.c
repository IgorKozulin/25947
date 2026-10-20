#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

typedef struct {
    off_t offset;
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

static int print_file(int fd) {
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        return 1;
    }

    char block[4096];

    for (;;) {
        ssize_t n = read(fd, block, sizeof(block));

        if (n == -1) {
            if (errno == EINTR) continue;
            perror("read");
            return 1;
        }

        if (n == 0) break;

        if (fwrite(block, 1, (size_t)n, stdout) != (size_t)n) {
            perror("fwrite");
            return 1;
        }
    }

    putchar('\n');
    return 0;
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
    size_t count = 0, capacity = 0, length = 0;
    off_t start = 0;
    char c;
    char *buffer = NULL;
    int status = 1;

    for (;;) {
        ssize_t result = read(fd, &c, 1);

        if (result == -1) {
            if (errno == EINTR) continue;
            perror("read");
            goto done;
        }

        if (result == 0 && length == 0) break;

        if (result > 0) {
            length++;
            if (c != '\n') continue;
        }

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
        lines[count].length = length;
        count++;
        length = 0;

        start = lseek(fd, 0, SEEK_CUR);
        if (start == (off_t)-1) {
            perror("lseek");
            goto done;
        }

        if (result == 0) break;
    }

    puts("Line Offset Length");

    for (size_t i = 0; i < count; i++) {
        printf("%zu %lld %zu\n", i + 1,
               (long long)lines[i].offset, lines[i].length);
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
            status = print_file(fd);
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

        if (selected.length == (size_t)-1) {
            fputs("Line too long\n", stderr);
            goto done;
        }

        buffer = malloc(selected.length + 1);
        if (buffer == NULL) {
            perror("malloc");
            goto done;
        }

        if (lseek(fd, selected.offset, SEEK_SET) == (off_t)-1) {
            perror("lseek");
            goto done;
        }

        size_t total = 0;

        while (total < selected.length) {
            ssize_t result = read(fd, buffer + total,
                                  selected.length - total);

            if (result == -1 && errno == EINTR) continue;

            if (result <= 0) {
                if (result == -1) perror("read");
                else fputs("Unexpected end of file\n", stderr);
                goto done;
            }

            total += (size_t)result;
        }

        buffer[total] = '\0';
        printf("%s", buffer);

        if (total > 0 && buffer[total - 1] != '\n') {
            putchar('\n');
        }

        free(buffer);
        buffer = NULL;
    }

    status = 0;

done:
    free(buffer);
    free(lines);

    if (close(fd) == -1) {
        perror("close");
        status = 1;
    }

    return status;
}
