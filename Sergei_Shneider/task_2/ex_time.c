#define _POSIX_C_SOURCE 200809L //Разрешает объявления POSIX-функций, включая setenv

#include <stdio.h> //Объявления puts, perror и fprintf
#include <stdlib.h> //Объявление setenv
#include <time.h> //Типы time_t, struct tm и функции времени

int main(void)
{
    if (setenv("TZ", "America/Los_Angeles", 1) == -1) { //Задаём TZ; 1 разрешает замену, -1 означает ошибку
        perror("setenv"); //Выводит название функции и описание последней ошибки
        return 1; //Завершает программу с кодом ошибки
    }
    tzset(); //Читает TZ и обновляет правила преобразования времени

    time_t now = time(NULL); //Получаем число секунд с начала эпохи Unix; NULL — без дополнительной записи по адресу
    if (now == (time_t)-1) { //Проверяем признак ошибки, приведённый к типу time_t
        perror("time");
        return 1;
    }

    struct tm *sp = localtime(&now); //Передаём адрес now; получаем указатель на дату и часы в выбранном поясе
    if (sp == NULL) { //NULL означает, что структуру времени получить не удалось
        perror("localtime");
        return 1;
    }

    char result[64]; //Массив для строки, включая завершающий нулевой символ
    if (strftime(result, sizeof(result), //Указываем массив для результата и его размер в байтах
                 "%d/%m/%Y %H:%M %Z", sp) == 0) { //Формируем дату из sp; 0 означает, что строка не сформирована
        fprintf(stderr, "Cannot format date and time\n"); //Печатаем сообщение в поток ошибок
        return 1;
    }

    puts(result); //Печатает готовую строку и добавляет перевод строки
    return 0; //Завершает программу с кодом успеха
}
