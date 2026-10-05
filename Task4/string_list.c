#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    char *line;
    struct node *next;
};

static char *
read_line(void)
{
    size_t capacity = BUFSIZ;
    size_t length = 0;
    char *line = malloc(capacity);

    if (line == NULL) {
        return NULL;
    }
    line[0] = '\0';

    for (;;) {
        size_t available = capacity - length;
        size_t part_length;

        if (available > (size_t)INT_MAX) {
            available = (size_t)INT_MAX;
        }
        if (fgets(line + length, (int)available, stdin) == NULL) {
            if (ferror(stdin)) {
                free(line);
                return NULL;
            }
            if (length == 0) {
                free(line);
                return NULL;
            }
            return line;
        }

        part_length = strlen(line + length);
        length += part_length;
        if (length > 0 && line[length - 1] == '\n') {
            return line;
        }
        if (feof(stdin)) {
            return line;
        }

        if (capacity > (size_t)INT_MAX / 2) {
            free(line);
            errno = EOVERFLOW;
            return NULL;
        }
        capacity *= 2;
        {
            char *larger = realloc(line, capacity);
            if (larger == NULL) {
                free(line);
                return NULL;
            }
            line = larger;
        }
    }
}

static int
append_line(struct node **head, struct node **tail, const char *line)
{
    size_t length = strlen(line);
    struct node *item = malloc(sizeof(*item));

    if (item == NULL) {
        return -1;
    }
    item->line = malloc(length + 1);
    if (item->line == NULL) {
        free(item);
        return -1;
    }
    memcpy(item->line, line, length + 1);
    item->next = NULL;

    if (*tail == NULL) {
        *head = item;
    } else {
        (*tail)->next = item;
    }
    *tail = item;
    return 0;
}

static void
free_list(struct node *head)
{
    while (head != NULL) {
        struct node *next = head->next;
        free(head->line);
        free(head);
        head = next;
    }
}

int
main(void)
{
    struct node *head = NULL;
    struct node *tail = NULL;
    char *line;

    puts("Enter lines; a line beginning with '.' ends input.");
    for (;;) {
        errno = 0;
        line = read_line();
        if (line == NULL) {
            if (errno != 0 || ferror(stdin)) {
                if (errno != 0) {
                    perror("reading input");
                } else {
                    fprintf(stderr, "Error reading input\n");
                }
                free_list(head);
                return EXIT_FAILURE;
            }
            break;
        }
        if (line[0] == '.') {
            free(line);
            break;
        }
        if (append_line(&head, &tail, line) == -1) {
            perror("allocating list node");
            free(line);
            free_list(head);
            return EXIT_FAILURE;
        }
        free(line);
    }

    for (struct node *item = head; item != NULL; item = item->next) {
        size_t length = strlen(item->line);
        fputs(item->line, stdout);
        if (length == 0 || item->line[length - 1] != '\n') {
            putchar('\n');
        }
    }

    free_list(head);
    return EXIT_SUCCESS;
}
