#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 

struct node
{
    char *str;         // указатель на строку, которая хранится в этом узле
    struct node *next; // указатель на следующий узел
};


int main(void)
{
    struct node *head = NULL; // первый узел списка
    struct node *tail = NULL; // последний узел списка

    char buf[1024]; // буфер, в который читаем очередную строку
    while (fgets(buf, sizeof(buf), stdin) != NULL) { // Ввод. Пока есть строки для чтения 
        if (buf[0] == '.') { // если строка начинается с точки, заканчиваем ввод
            break;
        }

        size_t len = strlen(buf); // длина строки
        char *copy = malloc(len + 1); // выделяем память под копию строки (+1 байт для '\0')
        if (copy == NULL) { // malloc вернул NULL - память не выделилась
            perror("malloc"); // печатаем сообщение об ошибке
            return 1;
        }
        strcpy(copy, buf); // копируем строку из буфера в выделенную память

        struct node *n = malloc(sizeof(struct node)); // выделяем память под новый узел
        if (n == NULL) // память под узел не выделилась
        {
            perror("malloc"); // печатаем сообщение об ошибке
            free(copy);       // освобождаем уже выделенную память под строку
            return 1;
        }

        n->str = copy;  // узел указывает на свою копию строки
        n->next = NULL; // новый узел пока последний, следующего нет
        if (head == NULL) // список пуст
        {
            head = n; // новый узел становится первым
        }
        else
        {
            tail->next = n; // прицепляем новый узел за последним
        }
        tail = n; // новый узел теперь последний
    }

    struct node *cur = tail; // начинаем с первого узла
    while (cur != NULL) // выводим все строки
    {
        printf("%s", cur->str); // '\n' уже есть в конце строки, его оставил fgets
        cur = cur->next;        // переходим к следующему узлу
    }

    cur = head; // снова встаём на начало списка
    while (cur != NULL) // освобождаем память
    {
        struct node *next = cur->next; // запоминаем следующий узел, пока текущий ещё не освобождён
        free(cur->str);                // освобождаем строку
        free(cur);                     // освобождаем сам узел
        cur = next;                    // переходим к следующему
    }
    return 0;
}