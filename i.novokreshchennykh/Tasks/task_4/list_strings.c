#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node 
{
    char *data;
    struct Node *next;
}Node;

void append(Node **head,char *str)
{
    Node *new_node = malloc(sizeof(Node));
    new_node -> data = malloc(strlen(str) + 1);
    strcpy(new_node -> data, str);
    new_node -> next = NULL;
    if (*head == NULL)
    {
        *head = new_node;
    }

    else
    {
        Node *current = *head;

        while (current -> next != NULL)
        {
        current = current -> next;
        }
        current -> next = new_node;
    }
}

void free_list(Node *head)
{
    while (head != NULL)
    {
        Node *temp = head -> next;
        free(head -> data);
        free(head);
        head = temp;
    }
}

int main(){
    Node *head = NULL;
    char buf[1024];

    while (1)
    {
        fgets(buf,sizeof(buf), stdin);
        int len = strlen(buf);

        if (len > 0 && buf[len - 1] == '\n')
        {
            buf[len - 1] = '\0';
        }

        if (buf[0] == '.'){
            break;
        }
        append(&head, buf);
    }
    Node *current = head;

    int num = 1;
    while (current != NULL)
    {

        if (num % 2 == 0)
        {
            printf("%s\n", current -> data);
        }
        current = current -> next;
        num++;
    }
    free_list(head);
    
    return 0;
}
