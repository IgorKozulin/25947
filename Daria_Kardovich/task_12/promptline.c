#include <errno.h> 
#include <string.h> 
#include <unistd.h> 
#include "shell.h" 

int promptline(char *prompt, char *line, int sizline) // Печатает приглашение и читает одну командную строку
{
    int n = 0; // Количество уже прочитанных символов

    if (write(STDOUT_FILENO, prompt, strlen(prompt)) == -1) { // Печатаем приглашение shell в терминал
        return -1; 
    }

    while (n < sizline - 1) { // Читаем данные пока в массиве line есть свободное место
        ssize_t result = read(STDIN_FILENO, line + n, (size_t)(sizline - 1 - n)); // Читаем символы с клавиатуры после уже прочитанного текста

        if (result == 0) { // Проверяем нажал ли пользователь Ctrl+D
            line[n] = '\0'; // Ставим конец строки
            return n; // Возвращаем число уже прочитанных символов
        }

        if (result == -1) { // Проверяем произошла ли ошибка чтения
            if (errno == EINTR) { // Проверяем прервал ли сигнал функцию read
                continue; // Повторяем чтение строки
            }

            return -1; // Возвращаем ошибку при другой проблеме чтения
        } 

        n += (int)result; // Увеличиваем число прочитанных символов
        line[n] = '\0';

        if (n >= 2 && line[n - 2] == '\\' && line[n - 1] == '\n') { // Проверяем есть ли обратный слеш перед Enter
            line[n - 2] = ' '; // Заменяем обратный слеш на пробел
            line[n - 1] = ' '; // Заменяем перевод строки на пробел
            continue; 
        } 

        return n; 
    }

    line[sizline - 1] = '\0'; 
    return n;
} 
