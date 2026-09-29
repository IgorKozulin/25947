#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node
{
    char *str;
    struct node *next;
};


int main(void)
{
    struct node *head = NULL; // первый узел
    struct node *tail = NULL; // последний узел

    char buf[1024];
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (buf[0] == '.') {
            break;
        }

        size_t len = strlen(buf);
        char *copy = malloc(len + 1);
        if (copy == NULL) {
            perror("malloc");
            return 1;
        }
        strcpy(copy, buf);

        struct node *n = malloc(sizeof(struct node));
        if (n == NULL)
        {
            perror("malloc");
            return 1;
        }
        
        n->str = copy;
        n->next = NULL;
        if (head == NULL)
        {
            head = n;
        }
        else
        {
            tail->next = n;
        }
        tail = n;
    }
    struct node *cur = head;
    while (cur != NULL)
    {
        printf("%s", cur->str);
        cur = cur->next;
    }
    
    cur = head;
    while (cur != NULL)
        {
            struct node *next = cur->next; // запоминаем следующий узел, пока текущий ещё не освобождён
            free(cur->str);                // освобождаем строку
            free(cur);                     // освобождаем сам узел
            cur = next;                    // переходим к следующему
        }
    return 0;
}