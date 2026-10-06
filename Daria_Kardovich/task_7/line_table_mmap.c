#include <fcntl.h>     
#include <stdio.h>     
#include <stdlib.h>   
#include <sys/mman.h>  
#include <sys/stat.h>  
#include <sys/types.h> 
#include <unistd.h>    

//структура для одной строки файла.
struct Line {
    off_t position; // Начало строки в файле
    size_t length;  // Длина строки
};


int main(int argc, char *argv[])
{
    const char *filename = "text.txt";

    int fd;   // Результат функции open()
    struct stat file_info; // Результат функции fstat()
    char *file_data; // Результат функции mmap()

    struct Line *table = NULL;
    size_t count = 0;
    size_t line_start = 0;
    int line_number;

    if (argc == 2) {
        filename = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        return 1;
    }

    fd = open(filename, O_RDONLY);

        perror("open");
        return 1;
    }


    // получает информацию об открытом файле
    // Нам нужен file_info.st_size — размер файла
    if (fstat(fd, &file_info) == -1) {
        perror("fstat");
        close(fd);

        return 1;
    }

    if (file_info.st_size == 0) {
        // ФУНКЦИЯ printf:
        // печатает текст на экран.
        printf("The file is empty.\n");

        close(fd);
        return 0;
    }

    // mmap
    // помещает содержимое файла в память
    // После неё file_data — это массив символов файла
    file_data = mmap(NULL, (size_t)file_info.st_size,
                     PROT_READ, MAP_PRIVATE, fd, 0);

    if (file_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // Он идёт по всем символам файла
    for (size_t i = 0; i < (size_t)file_info.st_size; i++) {

        // Если найден '\n', значит закончилась одна строка
        if (file_data[i] == '\n') {
            struct Line *new_table;

            new_table = realloc(table, (count + 1) * sizeof(struct Line));

            if (new_table == NULL) {
                perror("realloc");

                free(table);

                //  munmap
                // убирает файл из памяти
                munmap(file_data, (size_t)file_info.st_size);

                close(fd);
                return 1;
            }

            table = new_table;

            // Запоминаем, где начинается строка
            table[count].position = (off_t)line_start;

            // Запоминаем длину строки
            table[count].length = i - line_start;

            count++;

            // Начало следующей строки — после символа '\n'.
            line_start = i + 1;
        }
    }

    // Если последняя строка не закончилась '\n'.
    if (line_start < (size_t)file_info.st_size) {
        struct Line *new_table;

        // добавляет место для последней строки.
        new_table = realloc(table, (count + 1) * sizeof(struct Line));

        if (new_table == NULL) {
            perror("realloc");
            free(table);
            munmap(file_data, (size_t)file_info.st_size);
            close(fd);
            return 1;
        }

        table = new_table;
        table[count].position = (off_t)line_start;
        table[count].length = (size_t)file_info.st_size - line_start;
        count++;
    }

    // выводит заголовок
    printf("Line table:\n");

    // Выводим все найденные строки
    for (size_t i = 0; i < count; i++) {
        printf("Line %zu: position = %lld, length = %zu\n",
               i,
               (long long)table[i].position,
               table[i].length);
    }

    while (1) {
        printf("\nEnter line number (negative number to exit): ");

        if (scanf("%d", &line_number) != 1) {
            break;
        }

        if (line_number < 0) {
            break;
        }

        if ((size_t)line_number >= count) {
            printf("There is no line with this number.\n");
            continue;
        }
        printf("Selected line: %.*s\n",
               (int)table[line_number].length,
               file_data + table[line_number].position);
    }
    free(table);
    // удаляет отображение файла из памяти
    munmap(file_data, (size_t)file_info.st_size);
    close(fd);
    return 0;
}
