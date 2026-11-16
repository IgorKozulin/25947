#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Структура для одной строки файла
struct Line {
    off_t position; // Начало строки в файле
    size_t length;  // Длина строки
};

int main(int argc, char *argv[])
{
    const char *filename = "text.txt";

    int fd; // Результат open()
    struct stat file_info;// информация о файле
    char *file_data; // файл в памяти
    struct Line *table = NULL;
    size_t count = 0;

    // В задании 5 были position и length при чтении через read()
    // В задании 7 используем начало текущей строки в памяти
    size_t line_start = 0;
    int line_number;

    if (argc == 2) {
        filename = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        return 1;
    }
    fd = open(filename, O_RDONLY);
    
    // Проверяем, открылся ли файл
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // вместо чтения файла через read() сначала узнаём его размер
    if (fstat(fd, &file_info) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (file_info.st_size == 0) {
        printf("The file is empty.\n");
        close(fd);
        return 0;
    }
    // В задании 5 файл читался функцией read()
    // Теперь mmap() отображает весь файл в память
    // После этого file_data[i] — i-й символ файла
    file_data = mmap(NULL, (size_t)file_info.st_size,
                     PROT_READ, MAP_PRIVATE, fd, 0);

    if (file_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    // В задании 5 было: read(fd, &symbol, 1)
    // Теперь проходим по символам файла через file_data[i]
    for (size_t i = 0; i < (size_t)file_info.st_size; i++) {
        if (file_data[i] == '\n') {
            struct Line *new_table;

            new_table = realloc(table, (count + 1) * sizeof(struct Line));

            if (new_table == NULL) {
                perror("realloc");
                free(table);
                // отменяем отображение файла в память
                munmap(file_data, (size_t)file_info.st_size);

                close(fd);
                return 1;
            }

            table = new_table;

            // Запоминаем начало и длину найденной строки
            table[count].position = (off_t)line_start;
            table[count].length = i - line_start;
            count++;

            line_start = i + 1;
        }
    }

    // Добавляем последнюю строку, если файл не закончился символом '\n'
    if (line_start < (size_t)file_info.st_size) {
        struct Line *new_table;

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

    printf("Line table:\n");

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

        // В задании 5 здесь были lseek(), malloc(), read() и free()
        // Теперь строка уже находится в памяти в file_data
        printf("Selected line: %.*s\n",
               (int)table[line_number].length,
               file_data + table[line_number].position);
    }

    free(table);
    // В задании 5 этого не было, потому что не было mmap()
    // munmap() убирает отображение файла из памяти
    munmap(file_data, (size_t)file_info.st_size);

    close(fd);
    return 0;
}
