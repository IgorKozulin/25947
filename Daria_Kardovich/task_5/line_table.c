#include <stdio.h>    
#include <stdlib.h>  
#include <unistd.h>   
#include <fcntl.h> 
#include <sys/types.h> 

//  хранит информацию об одной строке
struct Line {
    off_t position; // Позиция начала строки в файле
    size_t length;  // Длина строки в символах
};

int main(int argc, char *argv[])
{
    const char *filename = "text.txt"; 
    int fd;// Файловый дескриптор открытого файла
    char symbol; // Сюда будет считываться один символ
    off_t position = 0;// Текущ позиция в файле
    size_t length = 0;// Длина текущстроки
    struct Line *table = NULL; // Таблица строк
    size_t count = 0;// Колво найденных строк
    int line_number; // Номер строки, который введёт пользователь

    // Если пользователь указал имя файла при запуске программы
    if (argc == 2) {
        filename = argv[1]; // Используем имя файла из командной строки
    }
    
    fd = open(filename, O_RDONLY);

    // Проверяем, удалось ли открыть файл
    if (fd == -1) {
        perror("open"); 
        return 1;     
    }

    // Читаем файл по одному символу, пока read успешно считывает символ
    while (read(fd, &symbol, 1) == 1) {
        // Если встретили символ перевода строки
        if (symbol == '\n') {
            struct Line *new_table; // Указатель на увеличенную таблицу

            // Увеличиваем таблицу, чтобы добавить ещё одну строку
            new_table = realloc(table, (count + 1) * sizeof(struct Line));

            // Проверяем, удалось ли выделить память
            if (new_table == NULL) {
                perror("realloc"); 
                free(table);       
                close(fd);       
                return 1;        
            }

            table = new_table; // Сохраняем адрес новой таблицы

            // Записываем, с какой позиции в файле началась строка
            table[count].position = position - length;

            // Записываем длину найденной строки
            table[count].length = length;

            count++; // Увеличиваем количество строк в таблице

            length = 0; // Начинаем считать длину следующей строки
        } else {
            length++; 
        }

        position++; // Переходим к следующей позиции в файле
    }

    // Если файл не заканчивается символом Enter, сохраняем последнюю строку
    if (length > 0) {
        struct Line *new_table; // Указатель на увеличенную таблицу

        // Увеличиваем таблицу для последней строки
        new_table = realloc(table, (count + 1) * sizeof(struct Line));

        // Проверяем, удалось ли выделить память
        if (new_table == NULL) {
            perror("realloc");
            free(table); 
            close(fd);     
            return 1;         
        }

        table = new_table; // Сохраняем адрес увеличенной таблицы

        // Записываем начало последней строки
        table[count].position = position - length;

        // Записываем длину последней строки
        table[count].length = length;

        count++; // Увеличиваем количество строк
    }

    printf("Line table:\n"); // Печатаем заголовок таблицы

    // Выводим информацию о каждой найденной строке
    for (size_t i = 0; i < count; i++) {
        printf("Line %zu: position = %lld, length = %zu\n",
               i,                        // Номер строки
               (long long)table[i].position, // Позиция начала строки
               table[i].length);          // Длина строки
    }

    // Бесконечно запрашиваем номер строки, пока пользователь не выйдет
    while (1) {
        printf("\nEnter line number (negative number to exit): ");

        // Считываем номер строки
        if (scanf("%d", &line_number) != 1) {
            break; // Если введено не число, выходим из цикла
        }

        // Если введено отрицательное число, завершаем программу
        if (line_number < 0) {
            break;
        }

        // Проверяем, существует ли строка с таким номером
        if ((size_t)line_number >= count) {
            printf("There is no line with this number.\n"); // Сообщаем об ошибке
            continue; // Возвращаемся к новому запросу номера
        }

        // Переходим к началу выбранной строки
        if (lseek(fd, table[line_number].position, SEEK_SET) == -1) {
            perror("lseek"); // Выводим ошибку перемещения по файлу
            break;        
        }

        // Выделяем память под выбранную строку и символ '\0'
        char *line = malloc(table[line_number].length + 1);

        // Проверяем, удалось ли выделить память.
        if (line == NULL) {
            perror("malloc");
            break;          
        }

        // Читаем из файла ровно длину выбранной строки.
        if (read(fd, line, table[line_number].length) == -1) {
            perror("read"); 
            free(line);    
            break;          /
        }

        // Добавляем конец строки, чтобы её можно было вывести через printf
        line[table[line_number].length] = '\0';

        // Печатаем выбранную строку
        printf("Selected line: %s\n", line);

        free(line); // Освобождаем память выбранной строки
    }

    free(table); 
    close(fd); 

    return 0; 
}
