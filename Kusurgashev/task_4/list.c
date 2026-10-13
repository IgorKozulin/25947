
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 4096

struct Node {
    char *text;
    struct Node *next;
};

void free_list(struct Node *head)
{
    while (head != NULL)
    {
        struct Node *next = head->next;
        free(head->text);
        free(head);
        head = next;
    }
}

int main(void)
{
    char buffer[MAX_LINE];
    struct Node *head = NULL;
    struct Node *tail = NULL;
    struct Node *current;

    printf("Enter strings:\n");

    while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        size_t len;
        struct Node *node;

        if (buffer[0] == '.')
            break;

        len = strlen(buffer);

        if (len > 0 && buffer[len - 1] == '\n')
            buffer[--len] = '\0';

        node = malloc(sizeof(*node));

        if (node == NULL)
        {
            perror("malloc");
            free_list(head);
            return 1;
        }

        node->text = malloc(len + 1);

        if (node->text == NULL)
        {
            perror("malloc");
            free(node);
            free_list(head);
            return 1;
        }

        strcpy(node->text, buffer);
        node->next = NULL;

        if (head == NULL)
            head = node;
        else
            tail->next = node;

        tail = node;
    }

    printf("\nSaved strings:\n");

    for (current = head; current != NULL; current = current->next)
        puts(current->text);

    free_list(head);

    return 0;
}

