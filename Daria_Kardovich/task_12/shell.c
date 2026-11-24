#include <stdio.h> 
#include "shell.h" 

char *infile; // Указатель на имя файла после символа <
char *outfile; // Указатель на имя файла после символа >
char *appfile; // Указатель на имя файла после символов >>
struct command cmds[MAXCMDS]; // Массив разобранных команд
char bkgrnd; // Флаг фонового режима при символе &

#ifdef DEBUG // Этот код работает только при сборке с флагом -DDEBUG
static void print_parsed_commands(int ncmds) // Функция печатает результат разбора строки
{
    for (int i = 0; i < ncmds; i++) { // Проходим по всем найденным командам
        for (int j = 0; cmds[i].cmdargs[j] != NULL; j++) { // Проходим по всем аргументам текущей команды
            fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n", // Выводим номер команды номер аргумента и его текст
                    i, j, cmds[i].cmdargs[j]); // Передаём значения для вывода
        } 

        fprintf(stderr, "cmds[%d].cmdflag = %o\n", // Выводим флаг текущей команды в восьмеричном виде
                i, cmds[i].cmdflag); // Передаём номер команды и её флаг
    } 

    fprintf(stderr, "bkgrnd = %d\n", bkgrnd); // Выводим есть ли фоновый режим

    if (infile != NULL) { // Проверяем был ли указан файл ввода после <
        fprintf(stderr, "infile = %s\n", infile); // Выводим имя файла ввода
    }

    if (outfile != NULL) { 
        fprintf(stderr, "outfile = %s\n", outfile); // Выводим имя файла вывода
    }

    if (appfile != NULL) { // Проверяем был ли указан файл дописывания после >>
        fprintf(stderr, "appfile = %s\n", appfile); // Выводим имя файла для дописывания
    } 
} 
#endif 

int main(int argc, char *argv[])
{
    char line[1024]; // Массив для введённой пользователем строки
    char prompt[50]; // Массив для текста приглашения shell
    int ncmds; // Переменная для количества найденных команд

    (void)argc; // Убираем предупреждение о неиспользуемом argc

    snprintf(prompt, sizeof(prompt), "[%s] ", argv[0]); // [./myshell]

    while (promptline(prompt, line, sizeof(line)) > 0) { // Показываем приглашение и читаем строки пока не нажмут Ctrl+D
        ncmds = parseline(line); // Разбираем введённую строку и получаем число команд

        if (ncmds <= 0) { // Проверяем что строка пустая или содержит синтаксическую ошибку
            continue; // Возвращаемся к следующему приглашению
        }

#ifdef DEBUG // Этот код работает только при сборке с флагом -DDEBUG
        print_parsed_commands(ncmds); // Выводим разобранные команды и специальные символы
#endif
    } 

    return 0; 
} 
