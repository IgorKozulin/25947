#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

struct Node
{
    char *text;
    struct Node *next;
};

int main(void)
{
    struct Node *head = NULL;
    struct Node *tail = NULL;
    char buffer[MAX_LINE];

    printf("Enter strings (line starting with '.' ends input):\n");

    while (fgets(buffer, MAX_LINE, stdin) != NULL)
    {
        size_t len;
        struct Node *new_node;

        if (buffer[0] == '.')
            break;

        len = strlen(buffer);

        if (len > 0 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
            len--;
        }

        new_node = (struct Node *)malloc(sizeof(struct Node));

        if (new_node == NULL)
        {
            perror("malloc");
            return 1;
        }

        new_node->text = (char *)malloc(len + 1);

        if (new_node->text == NULL)
        {
            perror("malloc");
            free(new_node);
            return 1;
        }

        strcpy(new_node->text, buffer);
        new_node->next = NULL;

        if (head == NULL)
        {
            head = new_node;
            tail = new_node;
        }
        else
        {
            tail->next = new_node;
            tail = new_node;
        }
    }

    printf("\nStrings from the list:\n");

    {
        struct Node *current = head;

        while (current != NULL)
        {
            printf("%s\n", current->text);
            current = current->next;
        }
    }

    while (head != NULL)
    {
        struct Node *temp = head;

        head = head->next;

        free(temp->text);
        free(temp);
    }

    return 0;
}
