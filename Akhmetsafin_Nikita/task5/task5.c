#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

struct Line {
    off_t offset;
    size_t length;
};

int main(int argc, char *argv[]) {
    int fd;
    char c;
    size_t current_len;
    struct Line *lines;
    size_t capacity;
    size_t line_count;
    int line_num;
    char *buffer;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    current_len = 0;
    capacity = 10;
    line_count = 0;

    lines = (struct Line *)malloc(capacity * sizeof(struct Line));
    if (lines == NULL) {
        perror("malloc error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    lines[line_count].offset = 0;

    while (read(fd, &c, 1) == 1) {
        current_len++;
        
        if (c == '\n') {
            lines[line_count].length = current_len;
            line_count++;
            
            if (line_count >= capacity) {
                capacity *= 2;
                lines = (struct Line *)realloc(lines, capacity * sizeof(struct Line));
                if (lines == NULL) {
                    perror("realloc error");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            }
            
            lines[line_count].offset = lseek(fd, 0L, SEEK_CUR);
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
        
        if (scanf("%d", &line_num) != 1) {
            break;
        }
        
        if (line_num == 0) {
            break;
        }
        
        if (line_num < 0 || line_num > line_count) {
            printf("Invalid line number. Max: %lu\n", (unsigned long)line_count);
            continue;
        }
        
        buffer = (char *)malloc(lines[line_num - 1].length + 1);
        if (buffer == NULL) {
            perror("malloc error");
            continue;
        }
        
        lseek(fd, lines[line_num - 1].offset, SEEK_SET);
        read(fd, buffer, lines[line_num - 1].length);
        buffer[lines[line_num - 1].length] = '\0';
        
        printf("%s", buffer);
        if (lines[line_num - 1].length > 0 && buffer[lines[line_num - 1].length - 1] != '\n') {
            printf("\n");
        }
        
        free(buffer);
    }

    free(lines);
    close(fd);
    return 0;
}
