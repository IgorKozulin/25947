#ifndef LINE_TABLE_H
#define LINE_TABLE_H

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

struct LineEntry {
    off_t offset;
    size_t length;
};

struct LineTable {
    int fd;
    struct LineEntry *lines;
    size_t count;
    size_t capacity;
};

static inline int add_line(struct LineTable *table, off_t offset, size_t length)
{
    if (table->count == table->capacity) {
        size_t next = table->capacity == 0 ? 16 : table->capacity * 2;
        struct LineEntry *grown;
        if (next < table->capacity || next > SIZE_MAX / sizeof(*grown)) {
            errno = ENOMEM;
            return -1;
        }
        grown = realloc(table->lines, next * sizeof(*grown));
        if (grown == NULL)
            return -1;
        table->lines = grown;
        table->capacity = next;
    }
    table->lines[table->count++] = (struct LineEntry){offset, length};
    return 0;
}

static inline int open_line_table(struct LineTable *table, const char *path)
{
    char buffer[4096];
    off_t position = 0, start = 0;
    ssize_t received;
    table->fd = open(path, O_RDONLY);
    if (table->fd == -1)
        return -1;
    while ((received = read(table->fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < received; ++i) {
            ++position;
            if (buffer[i] == '\n') {
                if (add_line(table, start, (size_t)(position - start)) == -1)
                    return -1;
                start = position;
            }
        }
    }
    if (received == -1)
        return -1;
    if (position > start && add_line(table, start, (size_t)(position - start)) == -1)
        return -1;
    return 0;
}

static inline int show_line(struct LineTable *table, size_t number)
{
    char buffer[4096];
    size_t remaining;
    char last = '\n';
    if (lseek(table->fd, table->lines[number].offset, SEEK_SET) == (off_t)-1)
        return -1;
    remaining = table->lines[number].length;
    while (remaining > 0) {
        size_t wanted = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        ssize_t received = read(table->fd, buffer, wanted);
        if (received <= 0) {
            if (received == 0)
                errno = EIO;
            return -1;
        }
        last = buffer[received - 1];
        if (fwrite(buffer, 1, (size_t)received, stdout) != (size_t)received)
            return -1;
        remaining -= (size_t)received;
    }
    if (last != '\n' && putchar('\n') == EOF)
        return -1;
    return 0;
}

static inline int show_whole_file(struct LineTable *table)
{
    char buffer[4096];
    ssize_t received;
    if (lseek(table->fd, 0, SEEK_SET) == (off_t)-1)
        return -1;
    while ((received = read(table->fd, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, (size_t)received, stdout) != (size_t)received)
            return -1;
    }
    return received == -1 ? -1 : 0;
}

static inline void close_line_table(struct LineTable *table)
{
    if (table->fd != -1)
        close(table->fd);
    free(table->lines);
}

#endif
