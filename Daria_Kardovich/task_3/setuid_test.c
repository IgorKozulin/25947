#include <stdio.h> 
#include <stdlib.h>  
#include <unistd.h>
#include <sys/types.h>  

// Главная функция программы.
int main() {
    FILE *fp; // Указатель на файл

    //UID пользователя, который запустил программу
    printf("Real UID: %d\n", getuid());

    // UID, по которому ОС сейчас проверяет права доступа
    printf("Effective UID: %d\n", geteuid());

    fp = fopen("data.txt", "r");

    // Если файл не удалось открыть
    if (fp == NULL) {
        perror("fopen"); 
    } else {
        fclose(fp); // Если файл открылся, сразу закрываем его
    }

    // Устанавливаем эффективный UID равным UID пользователя, запустившего программу
    if (setuid(getuid()) == -1) {
        perror("setuid"); 
        exit(EXIT_FAILURE); // Завершаем программу с ошибкой
    }

    // Ещё раз печатаем реальный UID
    printf("Real UID: %d\n", getuid());

    // Печатаем эффективный UID после вызова setuid
    printf("Effective UID: %d\n", geteuid());

    // Ещё раз пытаемся открыть data.txt, но уже с изменёнными правами
    fp = fopen("data.txt", "r");

    // Проверяем, открылся ли файл
    if (fp == NULL) {
        perror("fopen"); // Если прав нет, выводим сообщение об ошибке
    } else {
        fclose(fp); // Если файл открылся, закрываем его
    }

    return 0;
}
