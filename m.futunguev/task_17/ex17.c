#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define WIDTH 40 // ширина строки

struct termios orig; // исходные настройки терминала

void disable_raw(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); // вернуть исходные настройки
    // tcsetattr - terminal control set attributes, то есть «установить настройки терминала».
    // STDIN_FILENO — настрой тот терминал, с которого идёт ввод
    // TCSAFLUSH — дождаться вывода и выбросить всё, что пользователь успел напечатать, но что ещё не прочитано.
}

void enable_raw(void)
{
    tcgetattr(STDIN_FILENO, &orig); // считать текущие настройки в orig
    atexit(disable_raw); // вызвать disable_raw при завершении программы

    struct termios raw = orig; // копия настроек, которую будем менять.
    raw.c_lflag &= ~(ECHO | ICANON); // сбросить два флага.
    // ECHO — автопоказ символов
    // ICANON — построчный режим
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // применяем текущие настройки
}

void put(const char *s) // вывести С-строку целиком
{
    write(1, s, strlen(s));
}

int prev_rows = 0;
void redraw(const char *buf, int len)
{
    put("\r"); // возвращаем курсор в начало последней нарисованной строки

    for (int i = 0; i < prev_rows; i++)
    {
        put("\033[K"); // стереть текущую строку
        if (i < prev_rows - 1)
        {
            put("\033[A"); // подняться вверх (кроме самого верха)
        }
    }
    put("\r"); // курсор в начало самой верхней строки

    int col = 0;   // текущий столбец
    int rows = 1;  // сколько строк рисуем (хотя бы одна)
    int i = 0;

    while (i < len)
    {
        if (buf[i] == ' ')
        {
            // пробел: выводим, если помещается в строку
            if (col < WIDTH)
            {
                write(1, &buf[i], 1);
                col++;
            }
            i++;
        }
        else
        {
            // нашли начало слова: измерим его длину
            int wlen = 0;
            while (i + wlen < len && buf[i + wlen] != ' ')
                wlen++;

            // не влезает в остаток строки и не длиннее целой строки - переносим
            if (col + wlen > WIDTH && wlen <= WIDTH)
            {
                put("\n");
                col = 0;
                rows++;
            }

            // выводим слово посимвольно
            for (int k = 0; k < wlen; k++)
            {
                write(1, &buf[i + k], 1);
                col++;
                if (col == WIDTH) // слово длиннее строки - переносим прямо тут
                {
                    put("\n");
                    col = 0;
                    rows++;
                }
            }
            i += wlen;
        }
    }

    prev_rows = rows; // запомнили, сколько строк нарисовали
    
}

int main(void)
{
    enable_raw(); // переводим треминал в сырой режим

    char buf[1024]; // буфер текущей строки
    int pos = 0; // количество символов в строке
    char c; // прочитанный байт
    while (read(STDIN_FILENO, &c, 1) == 1) // пока прочитан 1 байт
    {
        switch (c)
        {
            case 4: // Ctrl+D - выход
                if (pos == 0) // по условию выход только на пустой строке
                {
                    disable_raw();
                    return 0;
                }
                break;

            case 127: // Backspace - стереть символ
            case 8:
                if (pos > 0)
                    pos--;
                break;

            case 21: // Ctrl+U - стереть строку
                pos = 0;
                break;

            case 23: // Ctrl+W - стереть слово
                while (pos > 0 && buf[pos - 1] == ' ') // пробелы перед курсором
                    pos--;
                while (pos > 0 && buf[pos - 1] != ' ') // само слово
                    pos--;
                break;

            case '\n': // Enter - новая строка
            case '\r':
                put("\n");
                pos = 0;
                prev_rows = 0; // новая строка рисуется с нуля
                break;

            default: // обычный символ
                if (isprint((unsigned char)c))
                {
                    if (pos < 1024)
                    {
                        buf[pos] = c;
                        pos++;
                    }
                }
                else
                {
                    put("\a"); // звонок
                }
                break;
        }

        redraw(buf, pos); // перерисовываем строку после любого изменения
    }
    return 0;
}

