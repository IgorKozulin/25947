#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 4096

struct node {
    char *str;
    struct node *next;
};

int main(void) {
    struct node *head = NULL, *tail = NULL;
    char buf[MAXLINE];

    printf("Enter lines (. to finish):\n");
    while (fgets(buf, sizeof(buf), stdin)) {
        if (buf[0] == '.') break;

        size_t len = strlen(buf);
        char *s = malloc(len + 1);
        if (!s) { perror("malloc"); exit(1); }
        memcpy(s, buf, len + 1);

        struct node *n = malloc(sizeof(*n));
        if (!n) { perror("malloc"); exit(1); }
        n->str = s;
        n->next = NULL;

        if (!head) head = tail = n;
        else { tail->next = n; tail = n; }
    }

    printf("\n--- Lines ---\n");
    for (struct node *p = head; p; p = p->next)
        printf("%s", p->str);

    while (head) {
        struct node *n = head->next;
        free(head->str);
        free(head);
        head = n;
    }
    return 0;
}
