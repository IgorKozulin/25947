#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *str;
    struct Node *next;
};

int main() {
    char buffer[1024];
    struct Node *head = NULL;
    struct Node *tail = NULL;
    struct Node *current = NULL;
    struct Node *new_node = NULL;
    struct Node *temp = NULL;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (buffer[0] == '.') {
            break;
        }

        new_node = (struct Node *)malloc(sizeof(struct Node));
        if (new_node == NULL) {
            perror("malloc error");
            exit(EXIT_FAILURE);
        }

        new_node->str = (char *)malloc(strlen(buffer) + 1);
        if (new_node->str == NULL) {
            perror("malloc error");
            free(new_node);
            exit(EXIT_FAILURE);
        }

        strcpy(new_node->str, buffer);
        new_node->next = NULL;

        if (head == NULL) {
            head = new_node;
            tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
    }

    current = head;
    while (current != NULL) {
        printf("%s", current->str);
        current = current->next;
    }

    current = head;
    while (current != NULL) {
        temp = current;
        current = current->next;
        free(temp->str);
        free(temp);
    }

    return 0;
}
