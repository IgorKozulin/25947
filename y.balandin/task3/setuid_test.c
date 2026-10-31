#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

// Функция вывода реального и эффективного UID
void print_uids(const char *stage) {
    printf("--- %s ---\n", stage);
    printf("Real UID      = %d\n", getuid());
    printf("Effective UID = %d\n", geteuid());
}

// Функция проверки доступа к файлу
void try_open(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        perror("Ошибка открытия файла");
    } else {
        printf("Файл '%s' успешно открыт.\n", filename);
        fclose(f);
    }
}

int main(void) {
    const char *filename = "data.txt";

    // 1. Вывод UID'ов до сброса привилегий
    print_uids("До сброса привилегий");

    // 2. Попытка открыть файл с текущими (возможно, повышенными) правами
    try_open(filename);

    // 3. Сброс привилегий: эффективный UID становится равным реальному
    if (setuid(getuid()) != 0) {
        perror("setuid");
        exit(EXIT_FAILURE);
    }

    // 4. Вывод UID'ов после сброса
    print_uids("После сброса привилегий");

    // 5. Повторная попытка открыть файл уже без привилегий
    try_open(filename);

    return 0;
}
