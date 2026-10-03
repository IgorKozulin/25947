#include <stdio.h>
#include <stdlib.h>     // exit
#include <fcntl.h>      // open
#include <unistd.h>     // read, lseek, close, write

#include <signal.h>

#define MAX_LINES 1000  // максимальное число строк в файле

// одна запись таблицы: где строка начинается и сколько в ней байт
struct line
{
    long start; // позиция начала строки от начала файла
    long len;   // длина строки в байтах, включая '\n'
};

struct line table[MAX_LINES]; // объявляем глобально, а не как в 5 задании
int nlines = 0;
int fd = -1; // дескриптор нужен обработчику сигнала

void on_timeout(int sig)
{
    (void)sig;
    char buf[1024];
    printf("\nTime out, full file:\n");
    for (int i = 0; i < nlines; i++) // по всем строкам
    {
        long len = table[i].len;
        if (len > (long)sizeof(buf))
            len = sizeof(buf);
        lseek(fd, table[i].start, 0);  // на начало строки
        long got = read(fd, buf, len); // читаем её
        if (got > 0)
            write(1, buf, got);        // печатаем
    }
    printf("\n");
    _exit(0);
}

int main(int argc, char* argv[])
{
    if (argc < 2) // проверка на переданные аргументы
    {
        printf("usage: %s file\n", argv[0]); // имя файла не передали
        return 1;
    }

        fd = open(argv[1], O_RDONLY); // без int - используем глобальную переменную
    if (fd == -1) // открыть не удалось
    {
        perror(argv[1]);
        return 1;
    }

    char c;                        // текущий прочитанный байт 
    nlines = 0;                // сколько строк уже записано
    long start = 0;                // начало текущей строки (первая строка - с нуля)

    // читаем файл по одному байту и ищем концы строк
    while (read(fd, &c, 1) == 1)
    {
        if (c == '\n') // дошли до конца строки
        {
            long pos = lseek(fd, 0L, 1);     // текущая позиция = начало следующей строки
            table[nlines].start = start;     // запоминаем, где строка началась
            table[nlines].len = pos - start; // длина = конец минус начало
            nlines++;
            start = pos;                     // следующая строка начнётся отсюда
        }
    }

    // последняя строка без '\n' в конце файла
    long fsize = lseek(fd, 0L, 1); // курсор уже в конце файла, узнаём его позицию
    if (fsize > start)             // между последним '\n' и концом файла ещё есть байты
    {
        table[nlines].start = start;
        table[nlines].len = fsize - start;
        nlines++;
    }

    // печатаем таблицу (для отладки, сверяем с od -c)
    for (int i = 0; i < nlines; i++)
    {
        printf("line %d: start=%ld len=%ld\n", i, table[i].start, table[i].len);
    }

    char buf[1024]; // буфер для прочитанной строки
    int n;          // номер строки, который ввёл пользователь

    signal(SIGALRM, on_timeout);

    while (1)
    {
        printf("\nline number (0 to quit): ");
        alarm(5); // ставим будильник на 5 секунд
        if (scanf("%d", &n) != 1) // не удалось прочитать число
            break;
        alarm(0); // успели ввести - сбрасываем будильник
        if (n == 0) // ноль завершает работу
            break;

        if (n < 1 || n > nlines) // номер вне диапазона
        {
            printf("out of range\n");
            continue;
        }

        struct line L = table[n - 1]; // нужная строка в таблице (нумерация с 1)

        long len = L.len;
        if (len > (long)sizeof(buf)) // не читаем больше, чем влезает в буфер
            len = sizeof(buf);

        lseek(fd, L.start, 0);           // переходим на начало строки (0 = SEEK_SET, от начала файла)
        long got = read(fd, buf, len);   // читаем ровно len байт
        if (got > 0)
            write(1, buf, got);          // печатаем прочитанное, вместе с '\n'
    }

    close(fd);
    return 0;
}