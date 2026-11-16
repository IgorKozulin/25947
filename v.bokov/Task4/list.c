#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data;
    struct Node *next;
};

void append(struct Node **head, const char *str)
{
    struct Node *new_node;
    struct Node *d;
    size_t len;

    new_node = malloc(sizeof(struct Node));
    if (new_node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    len = strlen(str);

    new_node->data = malloc(len + 1);
    if (new_node->data == NULL) {
        perror("malloc");
        free(new_node);
        exit(EXIT_FAILURE);
    }

    strcpy(new_node->data, str);
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    d = *head;

    while (d->next != NULL) {
        d = d->next;
    }

    d->next = new_node;
}

void print_list(struct Node *head)
{
    struct Node *d = head;

    while (d != NULL) {
        printf("%s\n", d->data);
        d = d->next;
    }
}

void free_list(struct Node *head)
{
    struct Node *d = head;

    while (d != NULL) {
        struct Node *next = d->next;

        free(d->data);
        free(d);

        d = next;
    }
}

int main(void)
{
    char buffer[1024];
    struct Node *head = NULL;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);

        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        if (buffer[0] == '.') {
            break;
        }

        if (buffer[0] != '\0') {
            append(&head, buffer);
        }
    }

    print_list(head);
    free_list(head);

    return 0;
}