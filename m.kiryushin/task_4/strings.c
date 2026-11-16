#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data;
    struct Node *next;
};

void append(struct Node **head,char *str){
    struct Node *new_node = malloc(sizeof(struct Node));
    new_node -> data = malloc(strlen(str) + 1);
    strcpy(new_node -> data, str);
    new_node -> next = NULL;
    if (*head == NULL){
        *head = new_node;
    }else{
        struct Node *current = *head;

        while (current -> next != NULL){
        current = current -> next;
        }
        current -> next = new_node;
    }
}

void free_list(struct Node *head){
    while (head != NULL){
        struct Node *temp = head -> next;
        free(head -> data);
        free(head);
        head = temp;
    }
}

int main(){
    struct Node *head = NULL;
    char buf[1024];

    while (1){
        fgets(buf,sizeof(buf), stdin);
        int len = strlen(buf);

        if (len > 0 && buf[len - 1] == '\n'){
            buf[len - 1] = '\0';
        }

        if (buf[0] == '.'){
            break;
        }
        append(&head, buf);
    }
    struct Node *current = head;

    while (current != NULL){
        printf("%s\n", current -> data);
        current = current -> next;
    }
    free_list(head);
    
    return 0;
}


