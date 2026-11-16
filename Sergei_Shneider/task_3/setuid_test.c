#include <stdio.h> //Объявления printf, puts, perror, fopen и fclose
#include <sys/types.h> //Системные типы, включая uid_t
#include <unistd.h> //Объявления getuid, geteuid и setuid

static void check_access(void) //Одна проверка: вывод идентификаторов и попытка открыть файл
{
    printf("UID=%lu EUID=%lu\n",
           (unsigned long)getuid(),
           (unsigned long)geteuid()); //Получаем оба UID; приводим к типу для формата %lu

    FILE *file = fopen("data.txt", "r"); //Открываем файл из текущей папки для чтения
    if (file == NULL) { //При неудаче fopen возвращает нулевой указатель
        perror("fopen"); //Печатаем причину, например Permission denied
        return; //Выходим из check_access; выполнение main продолжится
    }

    puts("data.txt opened successfully"); //Сообщаем, что открытие удалось

    if (fclose(file) == EOF) { //Закрываем файл; EOF означает ошибку закрытия
        perror("fclose");
    }
}

int main(void)
{
    puts("Before setuid:");
    check_access(); //Проверяем доступ с исходным эффективным UID

    if (setuid(getuid()) == -1) { //Передаём реальный UID, чтобы эффективный стал таким же
        perror("setuid"); //Печатаем причину неудачного изменения UID
        return 1; //Завершаемся, поскольку изменить UID не удалось
    }

    puts("After setuid:");
    check_access(); //Повторяем ту же проверку после изменения эффективного UID

    return 0; //Эксперимент завершён; отказ в открытии файла может быть ожидаемым
}
