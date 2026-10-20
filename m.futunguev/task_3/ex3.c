#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

void print_ids(void)
{
    printf("реальный UID %d\n", getuid());     // кто запустил программу
    printf("эффективный UID %d\n", geteuid()); // чьи права проверяются при доступе к файлам
}

void try_open(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror(path); // печатает причину ошибки
    }
    else
    {
        printf("файл %s успешно открыт\n", path);
        fclose(fp);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("usage: %s file\n", argv[0]);
        return 1;
    }

    print_ids();
    try_open(argv[1]);

    // делаем эффективный UID равным реальному
    if (setuid(getuid()) == -1)
    {
        perror("setuid");
        return 1;
    }

    print_ids();
    try_open(argv[1]);

    return 0;
}