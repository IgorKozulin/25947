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

        printf("Selected line: %.*s\n",
               (int)table[line_number].length,
               file_data + table[line_number].position);
    }

    free(table);
    munmap(file_data, (size_t)file_info.st_size);
    close(fd);

    return 0;
}
