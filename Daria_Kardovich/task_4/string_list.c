#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>    

//один элемент списка
struct Node {
    char *text; // Указатель на строку, которая хранится в этом элементе
    struct Node *next; // Указатель на след элемент списка
};

//освобождает всю память
void free_list(struct Node *head)
{
    // Пока тек элемент есть
    while (head != NULL) {
        struct Node *next = head->next; // запоминаем след элемент до удаления текущ

        free(head->text); 
        free(head);        

        head = next; // Переходим к след элементу
    }
}

int main(void)
{
    struct Node *head = NULL; 
    struct Node *tail = NULL; 
    char buffer[1024];     

    printf("Enter strings. Enter a single dot to finish.\n");
    
    // Считываем строки, пока пользователь вводит данные
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t length = strlen(buffer); // Нахождение длинs введённой строки

        // fgets сохраняет Enter как символ \n, поэтому удаляем его
        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[length - 1] = '\0'; 
            length--;
        }


        if (strcmp(buffer, ".") == 0) {
            break; 
        }

        //  для нового элемента списка.
        struct Node *new_node = malloc(sizeof(struct Node));

        // проверка - удалось ли выделить память
        if (new_node == NULL) {
            perror("malloc"); // Выводим сообщение об ошибке
            free_list(head);  // Освобождаем память уже созданного списка
            return 1;// Завершаем программу с кодом ошибки
        }

        // Выделяем памятьтпод введённую строку
        new_node->text = malloc(length + 1);

        // снова проверка
        if (new_node->text == NULL) {
            perror("malloc");
            free(new_node); 
            free_list(head); 
            return 1;     
        }
        // Копируем введённую строку из buffer в память нового элемента
        strcpy(new_node->text, buffer);


        new_node->next = NULL;

        // Если список пустой.
        if (head == NULL) {
            head = new_node; // Новый элемент становится первым элементом списка
        } else {
            tail->next = new_node; // Присоединяем новый элемент после последнего
        }

        tail = new_node; // Новый элемент теперь является последним в списке
    

    printf("\nSaved strings:\n"); // Печатаем заголовок перед выводом списка

    // Начинаем с первого элемента и идём до конца списка
    for (struct Node *current = head; current != NULL; current = current->next) {
        printf("%s\n", current->text); // Печатаем текст текущего элемента
    }

    free_list(head); 

    return 0;
}
