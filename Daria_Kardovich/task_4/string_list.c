#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *text;
    struct Node *next;
};

void free_list(struct Node *head)
{
    while (head != NULL) {
        struct Node *next = head->next;

        free(head->text);
        free(head);

        head = next;
    }
}

int main(void)
{
    struct Node *head = NULL;
    struct Node *tail = NULL;
    char buffer[1024];

    printf("Enter strings. Enter a single dot to finish.\n");

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t length = strlen(buffer);

        /* Убираем символ переноса строки, который fgets сохраняет. */
        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[length - 1] = '\0';
            length--;
        }

        /* Одна точка завершает ввод. */
        if (strcmp(buffer, ".") == 0) {
            break;
        }

        struct Node *new_node = malloc(sizeof(struct Node));
        if (new_node == NULL) {
            perror("malloc");
            free_list(head);
            return 1;
        }

        new_node->text = malloc(length + 1);
        if (new_node->text == NULL) {
            perror("malloc");
            free(new_node);
            free_list(head);
            return 1;
        }

        strcpy(new_node->text, buffer);
        new_node->next = NULL;

        if (head == NULL) {
            head = new_node;
        } else {
            tail->next = new_node;
        }

        tail = new_node;
    }

    printf("\nSaved strings:\n");

    for (struct Node *current = head; current != NULL; current = current->next) {
        printf("%s\n", current->text);
    }

    free_list(head);

    return 0;
}
