#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *line;
    struct Node *next;
};

int main(void)
{
    struct Node *head = NULL, *tail = NULL, *node;
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int failed = 0;

    while ((length = getline(&line, &capacity, stdin)) != -1) {
        if (line[0] == '.')
            break;
        node = malloc(sizeof(*node));
        if (node == NULL) {
            perror("malloc");
            failed = 1;
            break;
        }
        node->line = malloc((size_t)length + 1);
        if (node->line == NULL) {
            perror("malloc");
            free(node);
            failed = 1;
            break;
        }
        memcpy(node->line, line, (size_t)length + 1);
        node->next = NULL;
        if (tail == NULL)
            head = node;
        else
            tail->next = node;
        tail = node;
    }
    if (ferror(stdin)) {
        perror("getline");
        failed = 1;
    }
    free(line);
    for (node = head; node != NULL; node = node->next)
        fputs(node->line, stdout);
    while (head != NULL) {
        node = head->next;
        free(head->line);
        free(head);
        head = node;
    }
    if (ferror(stdout)) {
        perror("stdout");
        failed = 1;
    }
    return failed;
}
