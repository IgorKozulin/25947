#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data;
    struct Node *next;
};

int main(void)
{
    char buffer[1024];
    struct Node *head = NULL, *tail = NULL, *cur, *tmp;

    printf("Введите строки (точка в начале строки — конец ввода):\n");

    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            break;

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        if (buffer[0] == '.')
            break;

        struct Node *node = malloc(sizeof(struct Node));
        node->data = malloc(strlen(buffer) + 1);
        strcpy(node->data, buffer);
        node->next = NULL;

        if (head == NULL)
            head = tail = node;
        else {
            tail->next = node;
            tail = node;
        }
    }

    printf("\n--- Содержимое списка ---\n");
    for (cur = head; cur != NULL; cur = cur->next)
        printf("%s\n", cur->data);

    cur = head;
    while (cur != NULL) {
        tmp = cur->next;
        free(cur->data);
        free(cur);
        cur = tmp;
    }

    return 0;
}
