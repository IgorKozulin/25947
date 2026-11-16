#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

// Global variables for the signal handler
static char *global_filename = NULL;

void alarm_handler(int sig) {
    // Open file to print its full content
    int fd = open(global_filename, O_RDONLY);
    if (fd == -1) {
        _exit(EXIT_FAILURE);
    }

    printf("\n[TIMEOUT] Time is up! Printing full file content...\n");
    
    char buf[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, bytes_read);
    }
    
    close(fd);
    _exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    global_filename = argv[1];

    // Set up the signal handler
    signal(SIGALRM, alarm_handler);

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int num_lines = 0;
    int capacity = 10;
    LineInfo *table = (LineInfo *)malloc(capacity * sizeof(LineInfo));
    if (!table) {
        perror("Memory allocation error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char ch;
    ssize_t bytes_read;
    long current_pos = 0;
    long line_start = 0;
    int line_len = 0;

    while ((bytes_read = read(fd, &ch, 1)) > 0) {
        current_pos++;
        
        if (ch == '\n') {
            if (num_lines >= capacity) {
                capacity *= 2;
                LineInfo *temp = realloc(table, capacity * sizeof(LineInfo));
                if (!temp) {
                    perror("Memory reallocation error");
                    free(table);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                table = temp;
            }

            table[num_lines].offset = line_start;
            table[num_lines].length = line_len;
            num_lines++;

            line_start = current_pos;
            line_len = 0;
        } else {
            line_len++;
        }
    }

    if (line_len > 0) {
        if (num_lines >= capacity) {
            capacity *= 2;
            LineInfo *temp = realloc(table, capacity * sizeof(LineInfo));
            if (!temp) {
                perror("Memory reallocation error");
                free(table);
                close(fd);
                exit(EXIT_FAILURE);
            }
            table = temp;
        }
        table[num_lines].offset = line_start;
        table[num_lines].length = line_len;
        num_lines++;
    }

    if (bytes_read == -1) {
        perror("File read error");
        free(table);
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < num_lines; i++) {
        printf("Line %d: Offset = %ld, Length = %d\n", 
               i + 1, table[i].offset, table[i].length);
    }
    printf("-------------------------\n");

    int line_num;
    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5); // Start 5-second timer

        if (scanf("%d", &line_num) != 1) {
            alarm(0); // Cancel timer on invalid input
            while (getchar() != '\n'); // Clear buffer
            continue;
        }

        alarm(0); // Cancel timer on successful input

        if (line_num == 0) {
            break;
        }

        if (line_num < 1 || line_num > num_lines) {
            printf("Error: line number must be between 1 and %d\n", num_lines);
            continue;
        }

        LineInfo info = table[line_num - 1];
        
        if (lseek(fd, info.offset, SEEK_SET) == -1) {
            perror("Seek error");
            continue;
        }

        char *buffer = (char *)malloc(info.length + 1);
        if (!buffer) {
            perror("Buffer memory allocation error");
            continue;
        }

        ssize_t read_bytes = read(fd, buffer, info.length);
        if (read_bytes != info.length) {
            perror("Line read error");
            free(buffer);
            continue;
        }

        buffer[info.length] = '\0';
        
        printf("%s\n", buffer);
        
        free(buffer);
    }

    free(table);
    close(fd);

    return 0;
}