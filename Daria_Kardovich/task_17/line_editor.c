#include <ctype.h> 
#include <stdio.h>  
#include <stdlib.h> 
#include <termios.h> 
#include <unistd.h>
#define WIDTH 40

//Здесь сохраняются первоначальные настройки терминала
static struct termios original_termios;

// Флаг: изменили ли мы настройки терминала
static int terminal_changed = 0;

// Возвращает терминалу первоначальный обычный режим ввода
static void restore_terminal(void)
{
    // Если настройки были изменены
    if (terminal_changed) {
        // Восстанавливаем сохранённые настройки терминала
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    }
}

// Печатает size символов текста в терминал
static void print_text(const char *text, size_t size)
{
    // write выводит текст сразу, без буфера printf
    write(STDOUT_FILENO, text, size);
}

// Удаляет последний символ текущей строки
static void erase_last(char *line, size_t *length)
{
    // Удаляем только если строка не пустая
    if (*length > 0) {
        print_text("\b \b", 3); // \b — назад, пробел стирает символ, \b — снова назад
        // Уменьшаем длину строки
        (*length)--;
        line[*length] = '\0'; // Ставим признак конца строки
    }
}

// Находит индекс начала последнего слова в строке
static size_t find_word_start(const char *line, size_t length)
{
    // Идём с конца строки, пока встречаются символы слова
    while (length > 0 && !isspace((unsigned char)line[length - 1])) {
        length--;
    }
    // Возвращаем позицию начала последнего слова.
    return length;
}

// Главная функция программы.
int main(void)
{
    // Здесь будут настройки терминала для посимвольного ввода.
    struct termios raw_termios;
    // Массив для символов текущей строки
    // +1 нужен для символа конца строки '\0'
    char line[WIDTH + 1];

    // Текущая длина строки
    size_t length = 0;

    // Сюда read() будет записывать нажатую клавишу
    unsigned char symbol;

    // Клавиша удаления последнего символа
    unsigned char erase_key;

    // Клавиша удаления всей строки
    unsigned char kill_key;

    // Получаем текущие настройки терминала
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        return 1;
    }

    // Копируем обычные настройки в переменную для изменения
    raw_termios = original_termios;

    // Отключаем обычный построчный ввод и автоматический вывод символов
    raw_termios.c_lflag &= ~(ICANON | ECHO);

    // Отключаем автоматическую замену Enter и Ctrl + S/Ctrl + Q
    raw_termios.c_iflag &= ~(ICRNL | IXON);

    // read() вернёт результат после одного введённого символа
    raw_termios.c_cc[VMIN] = 1;

    // Не используем тайм-аут ожидания символа
    raw_termios.c_cc[VTIME] = 0;

    // Запоминаем клавишу Backspace из обычных настроек терминал
    erase_key = original_termios.c_cc[VERASE];

    // Запоминаем клавишу Ctrl + U из обычных настроек терминала
    kill_key = original_termios.c_cc[VKILL];

    // Включаем изменённый режим терминала
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios) == -1) {
        perror("tcsetattr");
        return 1;
    }

    // Отмечаем, что терминал был изменён
    terminal_changed = 1;

    // При любом нормальном завершении программы вернём старые настройки
    atexit(restore_terminal);

    // Печатаем инструкцию пользователю
    print_text("Line editor. Ctrl-D on an empty line exits.\r\n", 43);

    // Читаем клавиши по одной
    while (read(STDIN_FILENO, &symbol, 1) == 1) {

        // Ctrl + D имеет код 4
        if (symbol == 4) {

            // На пустой строке Ctrl + D завершает программу
            if (length == 0) {
                print_text("\r\n", 2);
                break;
            }

            // На непустой строке подаём звуковой сигнал
            print_text("\a", 1);
            continue;
        }

        // Обрабатываем Enter
        if (symbol == '\r' || symbol == '\n') {
            // Переходим на новую строку
            print_text("\r\n", 2);

            // Новая строка пока пустая.
            length = 0
            line[0] = '\0';
            continue;
        }

        // Обрабатываем Backspace
        if (symbol == erase_key || symbol == 127 || symbol == 8) {
            erase_last(line, &length);
            continue;
        }

        // Обрабатываем Ctrl + U: удалить всю текущую строку.
        if (symbol == kill_key || symbol == 21) {
            // Удаляем символы, пока строка не станет пустой.
            while (length > 0) {
                erase_last(line, &length);
            }

            continue;
        }

        // Ctrl + W имеет код 23: удалить последнее слово.
        if (symbol == 23) {

            // Сначала удаляем пробелы справа от слова.
            while (length > 0 && isspace((unsigned char)line[length - 1])) {
                erase_last(line, &length);
            }

            // Затем удаляем символы последнего слова.
            while (length > 0 && !isspace((unsigned char)line[length - 1])) {
                erase_last(line, &length);
            }

            continue;
        }

        // Проверяем, что символ можно печатать.
        if (!isprint(symbol) && symbol != ' ') {
            // Для других управляющих клавиш подаём звуковой сигнал.
            print_text("\a", 1);
            continue;
        }

        // Если длина строки достигла 40 символов.
        if (length == WIDTH) {

            // Ищем начало последнего слова.
            size_t word_start = find_word_start(line, length);

            // Если последнее слово началось не в начале строки.
            if (word_start > 0) {
                // Вычисляем длину последнего слова.
                size_t word_length = length - word_start;

                // Временно сохраняем последнее слово.
                char word[WIDTH + 1];

                // Копируем слово во временный массив.
                for (size_t i = 0; i < word_length; i++) {
                    word[i] = line[word_start + i];
                }

                // Завершаем временную строку.
                word[word_length] = '\0';

                // Удаляем последнее слово с текущей строки на экране.
                while (length > word_start) {
                    erase_last(line, &length);
                }

                // Переходим на следующую строку.
                print_text("\r\n", 2);

                // Копируем слово в начало новой строки.
                for (size_t i = 0; i < word_length; i++) {
                    line[i] = word[i];
                }

                // Устанавливаем новую длину строки.
                length = word_length;

                // Завершаем новую строку.
                line[length] = '\0';

                // Печатаем перенесённое слово.
                print_text(line, length);
            } else {
                // Если слово занимает всю строку, просто переходим вниз.
                print_text("\r\n", 2);

                // Начинаем новую пустую строку.
                length = 0;
                line[0] = '\0';
            }
        }

        // Добавляем введённый символ в массив строки.
        line[length] = (char)symbol;

        // Увеличиваем длину строки.
        length++;

        // Ставим конец строки.
        line[length] = '\0';

        // Сразу выводим введённый символ.
        write(STDOUT_FILENO, &symbol, 1);
    }
    return 0;
}
 
