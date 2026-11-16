#define __EXTENSIONS__ 1
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct Node {
    char *text;
    struct Node *next;
} Node;

int main(void) {
    Node *head = NULL;
    Node *tail = NULL;
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int status = 1;

    while ((length = getline(&line, &capacity, stdin)) != -1) {
        if (line[0] == '.') {
            break;
        }

        Node *node = malloc(sizeof(*node));
        if (node == NULL) {
            perror("malloc");
            goto done;
        }

        node->text = malloc(strlen(line) + 1);
        if (node->text == NULL) {
            perror("malloc");
            free(node);
            goto done;
        }

        strcpy(node->text, line);
        node->next = NULL;

        if (tail == NULL) {
            head = node;
        } else {
            tail->next = node;
        }
        tail = node;
    }

    if (length == -1 && !feof(stdin)) {
        perror("getline");
        goto done;
    }

    for (Node *node = head; node != NULL; node = node->next) {
        fputs(node->text, stdout);
    }
    status = 0;

done:
    free(line);
    while (head != NULL) {
        Node *next = head->next;
        free(head->text);
        free(head);
        head = next;
    }
    return status;
}
