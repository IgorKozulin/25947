#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data;
    struct Node *next;
};

void append(struct Node **head, const char *str) {
    struct Node *new_node = malloc(sizeof(struct Node));
    if (!new_node) exit(1);
    new_node->data = malloc(strlen(str) + 1);
    if (!new_node->data) exit(1);
    strcpy(new_node->data, str);
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        struct Node *cur = *head;
        while (cur->next) cur = cur->next;
        cur->next = new_node;
    }
}

void free_list(struct Node *head) {
    while (head) {
        struct Node *next = head->next;
        free(head->data);
        free(head);
        head = next;
    }
}

int main(void) {
    char buffer[1024];
    struct Node *head = NULL;

    while (1) {
        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        if (buffer[0] == '.') break;
        append(&head, buffer);
    }

    for (struct Node *cur = head; cur; cur = cur->next)
        printf("%s\n", cur->data);

    free_list(head);
    return 0;
}
