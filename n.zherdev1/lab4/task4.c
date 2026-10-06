#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *text;
    struct Node *next;
};

int main(void)
{
    char buffer[1024];

    struct Node *head = NULL;
    struct Node *tail = NULL;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (buffer[0] == '.')
            break;

        struct Node *node = malloc(sizeof(struct Node));
        node->text = malloc(strlen(buffer) + 1);

        strcpy(node->text, buffer);
        node->next = NULL;

        if (head == NULL)
            head = node;
        else
            tail->next = node;

        tail = node;
    }

    for (struct Node *node = head; node != NULL; node = node->next)
        printf("%s", node->text);

    while (head != NULL) {
        struct Node *next = head->next;
        free(head->text);
        free(head);
        head = next;
    }

    return 0;
}
