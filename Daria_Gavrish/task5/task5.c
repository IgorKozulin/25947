
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define MAX_LINES 1000
#define BUFFER_SIZE 1

struct Line {
    off_t offset;
    size_t length;
};

int main(void)
{
    int fd;
    char ch;
    ssize_t bytes_read;
    off_t position;
    struct Line lines[MAX_LINES];
    int count = 0;
    off_t line_start = 0;
    size_t line_length = 0;
    int line_number;

    fd = open("text.txt", O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    position = lseek(fd, 0L, SEEK_CUR);

    if (position == (off_t)-1) {
        perror("lseek");
        close(fd);
        return 1;
    }

    while ((bytes_read = read(fd, &ch, 1)) > 0) {
        if (ch == '\n') {
            if (count >= MAX_LINES) {
                fprintf(stderr, "Too many lines\n");
                close(fd);
                return 1;
            }

            lines[count].offset = line_start;
            lines[count].length = line_length;
            count++;

            position = lseek(fd, 0L, SEEK_CUR);

            if (position == (off_t)-1) {
                perror("lseek");
                close(fd);
                return 1;
            }

            line_start = position;
            line_length = 0;
        } else {
            line_length++;
        }
    }

    if (bytes_read == -1) {
        perror("read");
        close(fd);
        return 1;
    }

    /* Сохраняем последнюю строку, даже если файл не заканчивается \n. */
    if (line_length > 0) {
        if (count >= MAX_LINES) {
            fprintf(stderr, "Too many lines\n");
            close(fd);
            return 1;
        }

        lines[count].offset = line_start;
        lines[count].length = line_length;
        count++;
    }

    printf("Line table:\n");
    printf("Number\tOffset\tLength\n");

    {
        int i;
        for (i = 0; i < count; i++) {
            printf("%d\t%lld\t%lu\n",
                   i + 1,
                   (long long)lines[i].offset,
                   (unsigned long)lines[i].length);
        }
    }

    while (1) {
        char *buffer;
        size_t length;
        size_t total = 0;

        printf("\nEnter line number (0 to exit): ");
        fflush(stdout);

        if (scanf("%d", &line_number) != 1) {
            printf("\nInvalid input.\n");
            break;
        }

        if (line_number == 0)
            break;

        if (line_number < 1 || line_number > count) {
            printf("No such line. Enter a number from 1 to %d.\n", count);
            continue;
        }

        length = lines[line_number - 1].length;
        buffer = malloc(length + 1);

        if (buffer == NULL) {
            perror("malloc");
            close(fd);
            return 1;
        }

        if (lseek(fd, lines[line_number - 1].offset, SEEK_SET) == (off_t)-1) {
            perror("lseek");
            free(buffer);
            close(fd);
            return 1;
        }

        while (total < length) {
            bytes_read = read(fd, buffer + total, length - total);

            if (bytes_read < 0) {
                if (errno == EINTR)
                    continue;

                perror("read");
                free(buffer);
                close(fd);
                return 1;
            }

            if (bytes_read == 0)
                break;

            total += (size_t)bytes_read;
        }

        buffer[total] = '\0';
        printf("Line %d: %s\n", line_number, buffer);
        free(buffer);
    }

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    return 0;
}

