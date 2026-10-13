#include <fcntl.h>    
#include <signal.h>   
#include <stdio.h>    
#include <stdlib.h>    
#include <sys/types.h> 
#include <unistd.h>    


//хранит информацию об одной строке файла
struct Line {
    off_t position;   а
    size_t length;     
};


// Флаг, который показывает, истекли ли 5 секунд
// volatile нужен, потому что значение меняется внутри обработчика сигнала
// sig_atomic_t безопасно использовать при работе с сигналами
static volatile sig_atomic_t timed_out = 0;


// обработчик сигнала SIGALRM
void alarm_handler(int signal_number)
{
    //Эта строка убирает предупреждение компилятора
    (void)signal_number;
    timed_out = 1;  // Устанавливаем флаг: время вышло
}


// для вывода всего содержимого файла
void print_file(int fd)
{
    // Буфер, куда будем читать данные из файла частями
    char buffer[1024];

    // Колво реально прочитанных байтов
    ssize_t bytes_read;
    // Перемещаем указатель файла в самое начало
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        return;
    }


    // Читаем файл частями по максимум 1024 байта
    // read() возвращает количество прочитанных байтов
    // Когда файл закончится, read() вернет 0
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        // Выводим прочитанные байты на экран
        fwrite(buffer, 1, (size_t)bytes_read, stdout);
    }
}

int main(int argc, char *argv[])
{
    const char *filename = "text.txt";

    int fd;
    char symbol; //для одного символа
    off_t position = 0;  // Текущ позиция в файле
    size_t length = 0; // Длина текущей строки

    // Динамический массив структур Line - храниться информация обо всех строках
    struct Line *table = NULL;
    // Колво найденных строк
    size_t count = 0;
    // Номер строки, который введет пользователь
    int line_number;
    // Структура для настройки обработчика сигнала
    struct sigaction action;

    if (argc == 2) {
        filename = argv[1];
    }

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }


    // Читаем файл по одному символу
    // Цикл работает, пока удалось прочитать 1 символ
    while (read(fd, &symbol, 1) == 1) {
        if (symbol == '\n') {
            // Временный указатель для realloc()
            struct Line *new_table;


            // Увеличиваем массив table на один элемент
            new_table = realloc(table,(count + 1) * sizeof(struct Line));
            // Если память выделить не удалось
            if (new_table == NULL) {
                perror("realloc");
                free(table);
                close(fd);
                return 1;
            }


            // Теперь table указывает на новый увеличенный массив
            table = new_table;
            table[count].position = position - length;
            table[count].length = length;
            count++;
            length = 0;
        } else {
            length++;
        }

        position++;
    }

    if (length > 0) {
        struct Line *new_table;

        new_table = realloc(
            table,
            (count + 1) * sizeof(struct Line)
        );


        // Проверяем, удалось ли выделить память
        if (new_table == NULL) {
            perror("realloc");
            free(table);
            close(fd);
            return 1;
        }

        table = new_table;
        table[count].position = position - length;
        table[count].length = length;
        count++;
    }


    // Указываем функцию, которая будет вызвана
    // при получении сигнала SIGALRM
    action.sa_handler = alarm_handler;
    // Очищаем маску сигналов
    sigemptyset(&action.sa_mask);
    // Дополнительные флаги не используем
    action.sa_flags = 0;
    // Устанавливаем обработчик сигнала SIGALRM
    if (sigaction(SIGALRM, &action, NULL) == -1) {

        // Если установить обработчик не получилось,
        // выводим ошибку
        perror("sigaction");
        free(table);
        close(fd);
        return 1;
    }

    while (1) {
        int result;


        // Просим пользователя ввести номер строки
        printf("Enter line number within 5 seconds: ");
        // Принудительно выводим текст из буфера stdout на экран
        // Иначе приглашение иногда может не появиться сразу
        fflush(stdout);
        // Сбрасываем флаг таймера
        timed_out = 0;
        // Запускаем таймер на 5 секунд
        // Через 5 секунд процесс получит SIGALRM
        alarm(5);

        // Ждем, пока пользователь введет номер строки
        result = scanf("%d", &line_number);
        // Отменяем таймер, если пользователь успел ввести число
        alarm(0);
        // Проверяем, истекло ли время
        if (timed_out) {

            // Переходим на новую строку
            // и сообщаем, что время закончилось
            printf("\nTime is over. File contents:\n");

            // Выводим весь файл
            print_file(fd);
            free(table);
            close(fd);
            return 0;
        }
        if (result != 1 || line_number < 0) {
            break;
        }

        if ((size_t)line_number >= count) {
            printf("There is no line with this number.\n");
            continue;
        }

        if (lseek(
                fd,
                table[line_number].position,
                SEEK_SET
            ) == -1) {
            perror("lseek");
            break;
        }
        char *line = malloc(table[line_number].length + 1);
        if (line == NULL) {
            perror("malloc");
            break;
        }
        if (read(
                fd,
                line,
                table[line_number].length
            ) == -1) {
            perror("read");
            free(line);
            break;
        }
        line[table[line_number].length] = '\0';
        printf("Selected line: %s\n", line);
        free(line);
    }
    free(table);
    close(fd);
    return 0;
}

    return 0;
}
