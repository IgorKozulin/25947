#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct Line {
    off_t position;
    size_t length;
};

static volatile sig_atomic_t timed_out = 0;

static void alarm_handler(int signal_number)
{
    (void)signal_number;
    timed_out = 1;
}

int main(int argc, char *argv[])
{
    const char *filename = "text.txt";

    int fd;
    struct stat file_info;
    char *file_data;

    struct Line *table = NULL;
    size_t count = 0;
    size_t line_start = 0;
    int line_number;
    int status = 0;
    struct sigaction action = {0};

    if (argc == 2) {
        filename = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        return 1;
    }

    fd = open(filename, O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (fstat(fd, &file_info) == -1) {
        perror("fstat");
        close(fd);

        return 1;
    }

    if (!S_ISREG(file_info.st_mode)) {
        fprintf(stderr, "Expected a regular file.\n");
        close(fd);
        return 1;
    }

    if (file_info.st_size == 0) {

        printf("The file is empty.\n");

        close(fd);
        return 0;
    }

    file_data = mmap(NULL, (size_t)file_info.st_size,
                     PROT_READ, MAP_PRIVATE, fd, 0);

    if (file_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    for (size_t i = 0; i < (size_t)file_info.st_size; i++) {

        if (file_data[i] == '\n') {
            struct Line *new_table;

            new_table = realloc(table, (count + 1) * sizeof(struct Line));

            if (new_table == NULL) {
                perror("realloc");

                free(table);

                munmap(file_data, (size_t)file_info.st_size);

                close(fd);
                return 1;
            }

            table = new_table;

            table[count].position = (off_t)line_start;

            table[count].length = i - line_start;

            count++;

            line_start = i + 1;
        }
    }

    if (line_start < (size_t)file_info.st_size) {
        struct Line *new_table;

        new_table = realloc(table, (count + 1) * sizeof(struct Line));

        if (new_table == NULL) {
            perror("realloc");
            free(table);
            munmap(file_data, (size_t)file_info.st_size);
            close(fd);
            return 1;
        }

        table = new_table;
        table[count].position = (off_t)line_start;
        table[count].length = (size_t)file_info.st_size - line_start;
        count++;
    }

    printf("Line table:\n");

    for (size_t i = 0; i < count; i++) {
        printf("Line %zu: position = %lld, length = %zu\n",
               i,
               (long long)table[i].position,
               table[i].length);
    }

    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        free(table);
        munmap(file_data, (size_t)file_info.st_size);
        close(fd);
        return 1;
    }

    while (1) {
        printf("\nEnter line number within 5 seconds (negative number to exit): ");
        fflush(stdout);

        timed_out = 0;
        alarm(5);
        int result = scanf("%d", &line_number);
        alarm(0);

        if (timed_out) {
            printf("\nTime is over. File contents:\n");
            if (fwrite(file_data, 1, (size_t)file_info.st_size, stdout)
                    != (size_t)file_info.st_size) {
                perror("fwrite");
                status = 1;
            }
            break;
        }

        if (result != 1) {
            break;
        }

        if (line_number < 0) {
            break;
        }

        if ((size_t)line_number >= count) {
            printf("There is no line with this number.\n");
            continue;
        }
        printf("Selected line: ");
        if (fwrite(file_data + table[line_number].position, 1,
                   table[line_number].length, stdout)
                != table[line_number].length) {
            perror("fwrite");
            status = 1;
            break;
        }
        putchar('\n');
    }
    free(table);

    if (munmap(file_data, (size_t)file_info.st_size) == -1) {
        perror("munmap");
        status = 1;
    }
    close(fd);
    return status;
}
