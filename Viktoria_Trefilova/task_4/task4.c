#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char *text;
    struct Node *next;
} Node;

int main(void)
{
    char s[4096];
    Node *head = NULL, *tail = NULL, *p;
    int error = 0;

    while (fgets(s, sizeof(s), stdin)) {
        if (s[0] == '.') break;
        if (!strchr(s, '\n') && !feof(stdin)) {
            fputs("Line too long\n", stderr);
            error = 1;
            break;
        }
        p = malloc(sizeof(*p));
        if (!p) { perror("malloc"); error = 1; break; }
        p->text = malloc(strlen(s) + 1);
        if (!p->text) { free(p); perror("malloc"); error = 1; break; }
        strcpy(p->text, s);
        p->next = NULL;
        if (tail) tail->next = p;
        else head = p;
        tail = p;
    }
    if (ferror(stdin)) { perror("stdin"); error = 1; }
    while (head) {
        p = head;
        if (!error) fputs(p->text, stdout);
        head = head->next;
        free(p->text);
        free(p);
    }
    return error;
}
