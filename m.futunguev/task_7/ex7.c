#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // open
#include <unistd.h>     // close, write
#include <sys/mman.h>   // mmap, munmap
#include <sys/stat.h>   // fstat, struct stat

#define MAX_LINES 1000

struct line
{
    long start; // позиция начала строки от начала файла
    long len;   // длина строки в байтах, включая '\n'
};

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("usage: %s file\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY); // открываем файл на чтение
    if (fd == -1)
    {
        perror(argv[1]);
        return 1;
    }

    struct stat st;              // сюда fstat положит информацию о файле
    if (fstat(fd, &st) == -1)    // узнаём размер файла
    {
        perror("fstat");
        return 1;
    }
    long size = st.st_size;     // размер файла в байтах

    // отображаем файл в память: теперь addr[i] - это i-й байт файла
    char *addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    // проверка: выводим весь файл прямо из памяти
    struct line table[MAX_LINES];
    int nlines = 0; // номер ячейки таблицы
    long start = 0; // начало текущей строки

    for (long i = 0; i < size; i++) // идём по всем байтам файла
    {
        if (addr[i] == '\n') // конец строки
        {
            table[nlines].start = start;
            table[nlines].len = (i + 1) - start; // длина, включая '\n'
            nlines++;
            start = i + 1; // следующая строка начинается после '\n'
        }
    }
    // последняя строка без '\n' в конце файла
    if (start < size)
    {
        table[nlines].start = start;
        table[nlines].len = size - start;
        nlines++;
    }

    // печатаем таблицу для отладки
    for (int i = 0; i < nlines; i++)
    {
        printf("line %d: start=%ld len=%ld\n", i, table[i].start, table[i].len);
    }

        int n; // номер строки, который ввёл пользователь

    while (1)
    {
        printf("\nline number (0 to quit): ");
        if (scanf("%d", &n) != 1) // не число - выходим
            break;
        if (n == 0) // ноль завершает работу
            break;

        if (n < 1 || n > nlines) // номер вне диапазона
        {
            printf("out of range\n");
            continue;
        }

        struct line L = table[n - 1]; // нужная строка для (нумерация с 1)
        write(1, addr + L.start, L.len); // выводим прямо из памяти
    }

    munmap(addr, size); // убираем отображение
    close(fd);
    return 0;
}