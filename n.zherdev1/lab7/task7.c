#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LINES 1000

static volatile sig_atomic_t timed_out = 0;
static const char *data = NULL;
static size_t size = 0;

void on_alarm(int sig)
{
    (void)sig;
    timed_out = 1;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        return 1;

    int fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct stat st;

    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    size = st.st_size;

    if (size > 0) {
        data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

        if (data == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return 1;
        }
    }

    close(fd);

    size_t offsets[MAX_LINES];
    size_t lengths[MAX_LINES];
    int lines = 0;

    if (size > 0) {
        offsets[0] = 0;

        for (size_t i = 0; i < size; i++) {
            if (data[i] == '\n' && lines < MAX_LINES) {
                lengths[lines] = i + 1 - offsets[lines];
                lines++;

                if (lines < MAX_LINES)
                    offsets[lines] = i + 1;
            }
        }

        if (lines < MAX_LINES && offsets[lines] < size) {
            lengths[lines] = size - offsets[lines];
            lines++;
        }
    }

    signal(SIGALRM, on_alarm);

    while (1) {
        int number;

        printf("Line number: ");
        fflush(stdout);

        timed_out = 0;
        alarm(5);

        if (scanf("%d", &number) != 1) {
            alarm(0);

            if (timed_out) {
                printf("\n");
                if (size > 0)
                    printf("%.*s", (int)size, data);
            }

            break;
        }

        alarm(0);

        if (number == 0)
            break;

        if (number < 1 || number > lines) {
            printf("No such line\n");
            continue;
        }

        int i = number - 1;

        printf("%.*s", (int)lengths[i], data + offsets[i]);

        if (lengths[i] > 0 && data[offsets[i] + lengths[i] - 1] != '\n')
            printf("\n");
    }

    if (size > 0)
        munmap((void *)data, size);

    return 0;
}
