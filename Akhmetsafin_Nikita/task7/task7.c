#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>

struct Line {
    off_t offset;
    size_t length;
};

char *file_data;
off_t file_size;

void alarm_handler(int sig) {
    write(STDOUT_FILENO, "\n", 1);
    if (file_size > 0) {
        write(STDOUT_FILENO, file_data, file_size);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    int fd;
    struct stat sb;
    struct Line *lines;
    size_t capacity;
    size_t line_count;
    size_t current_len;
    off_t i;
    int line_num;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    if (fstat(fd, &sb) == -1) {
        perror("fstat error");
        close(fd);
        exit(EXIT_FAILURE);
    }
    file_size = sb.st_size;

    if (file_size == 0) {
        close(fd);
        return 0;
    }

    file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("mmap error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    signal(SIGALRM, alarm_handler);

    capacity = 10;
    line_count = 0;
    current_len = 0;

    lines = (struct Line *)malloc(capacity * sizeof(struct Line));
    if (lines == NULL) {
        perror("malloc error");
        munmap(file_data, file_size);
        close(fd);
        exit(EXIT_FAILURE);
    }

    lines[line_count].offset = 0;

    for (i = 0; i < file_size; i++) {
        current_len++;
        if (file_data[i] == '\n') {
            lines[line_count].length = current_len;
            line_count++;
            if (line_count >= capacity) {
                capacity *= 2;
                lines = (struct Line *)realloc(lines, capacity * sizeof(struct Line));
                if (lines == NULL) {
                    perror("realloc error");
                    munmap(file_data, file_size);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            }
            lines[line_count].offset = i + 1;
            current_len = 0;
        }
    }

    if (current_len > 0) {
        lines[line_count].length = current_len;
        line_count++;
    }

    while (1) {
        printf("Enter line number (0 to exit): ");
        fflush(stdout);
        
        alarm(5);
        if (scanf("%d", &line_num) != 1) {
            break;
        }
        alarm(0);
        
        if (line_num == 0) {
            break;
        }
        
        if (line_num < 0 || line_num > line_count) {
            printf("Invalid line number. Max: %lu\n", (unsigned long)line_count);
            continue;
        }
        
        write(STDOUT_FILENO, file_data + lines[line_num - 1].offset, lines[line_num - 1].length);
        if (lines[line_num - 1].length > 0 && file_data[lines[line_num - 1].offset + lines[line_num - 1].length - 1] != '\n') {
            write(STDOUT_FILENO, "\n", 1);
        }
    }

    free(lines);
    munmap(file_data, file_size);
    close(fd);
    return 0;
}
