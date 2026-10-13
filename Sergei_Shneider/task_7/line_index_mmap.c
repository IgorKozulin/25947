#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

typedef struct {
    size_t start;
    size_t length;
} Line;

static volatile sig_atomic_t timed_out = 0;

static void on_alarm(int sig)
{
    (void)sig;
    timed_out = 1;
    close(STDIN_FILENO);
}

int main(int argc, char **argv)
{
    int fd, status = 1, result;
    struct stat info;
    char *data = MAP_FAILED;
    Line *lines = NULL, *tmp;
    size_t size = 0, start = 0, pos, count = 0, number, i, length;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (fstat(fd, &info) == -1) {
        perror("fstat");
        goto done;
    }
    size = (size_t)info.st_size;

    if (size > 0) {
        data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            perror("mmap");
            goto done;
        }
    }

    for (pos = 0; pos < size; ++pos) {
        if (data[pos] != '\n' && pos + 1 != size) {
            continue;
        }

        tmp = realloc(lines, (count + 1) * sizeof(*lines));
        if (!tmp) {
            perror("realloc");
            goto done;
        }
        lines = tmp;

        lines[count].start = start;
        lines[count].length = pos + 1 - start;
        ++count;
        start = pos + 1;
    }

    for (i = 0; i < count; ++i) {
        printf("%zu: offset=%zu length=%zu\n", i + 1,
               lines[i].start, lines[i].length);
    }

    if (signal(SIGALRM, on_alarm) == SIG_ERR) {
        perror("signal");
        goto done;
    }

    alarm(5);

    for (;;) {
        printf("Line number (0 to quit): ");
        fflush(stdout);

        result = scanf("%zu", &number);
        alarm(0);

        if (timed_out) {
            putchar('\n');
            if (size > 0 && fwrite(data, 1, size, stdout) != size) {
                perror("fwrite");
                goto done;
            }
            status = 0;
            break;
        }

        if (result != 1) {
            if (feof(stdin)) {
                status = 0;
                break;
            }
            fputs("Invalid number\n", stderr);
            goto done;
        }

        if (number == 0) {
            status = 0;
            break;
        }

        if (number > count) {
            puts("No such line");
            continue;
        }

        start = lines[number - 1].start;
        length = lines[number - 1].length;

        if (fwrite(data + start, 1, length, stdout) != length) {
            perror("fwrite");
            goto done;
        }

        if (data[start + length - 1] != '\n') {
            putchar('\n');
        }
    }

done:
    alarm(0);
    free(lines);

    if (data != MAP_FAILED && munmap(data, size) == -1) {
        perror("munmap");
        status = 1;
    }

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }
    return status;
}
