#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>      // getpwuid
#include <grp.h>      // getgrgid
#include <time.h>     // localtime, strftime
#include <string.h>   // strcpy
#include <libgen.h>   // basename

// формируем строку прав rwxrwxrwx
void perms(mode_t mode, char *out) // out - буфер минимум на 10 символов
{
    // права владельца
    out[0] = (mode & S_IRUSR) ? 'r' : '-'; // чтение
    out[1] = (mode & S_IWUSR) ? 'w' : '-'; // запись
    out[2] = (mode & S_IXUSR) ? 'x' : '-'; // исполнение
    // права группы
    out[3] = (mode & S_IRGRP) ? 'r' : '-';
    out[4] = (mode & S_IWGRP) ? 'w' : '-';
    out[5] = (mode & S_IXGRP) ? 'x' : '-';
    // права остальных
    out[6] = (mode & S_IROTH) ? 'r' : '-';
    out[7] = (mode & S_IWOTH) ? 'w' : '-';
    out[8] = (mode & S_IXOTH) ? 'x' : '-';
    out[9] = '\0'; // завершаем строку
}

int main(int argc, char *argv[])
{
    if (argc < 2) // файл не передали
    {
        printf("usage: %s file...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) // по всем файлам из аргументов
    {
        struct stat st;
        if (lstat(argv[i], &st) == -1) // получаем данные о файле
        {
            perror(argv[i]); // файла нет или нет доступа
            continue;        // к следующему файлу
        }

        // тип файла
        char type;
        if (S_ISDIR(st.st_mode)) type = 'd';      // каталог
        else if (S_ISREG(st.st_mode)) type = '-'; // обычный файл
        else type = '?';                          // всё остальное

        // права rwxrwxrwx
        char rwx[10];
        perms(st.st_mode, rwx);

        // числовые id переводим в имена
        struct passwd *pw = getpwuid(st.st_uid); // владелец
        struct group  *gr = getgrgid(st.st_gid); // группа
        char *owner = pw ? pw->pw_name : "?"; // имя или "?", если не нашлось
        char *group = gr ? gr->gr_name : "?";

        // размер: только для обычного файла, иначе пусто
        char size[32];
        if (S_ISREG(st.st_mode))
            sprintf(size, "%ld", (long)st.st_size); // размер в строку
        else
            size[0] = '\0'; // пустая строка

        // дата изменения
        char date[32];
        struct tm *tm = localtime(&st.st_mtime);        // раскладываем время
        strftime(date, sizeof(date), "%b %e %H:%M", tm); // форматируем

        // имя файла без пути
        char namecopy[1024];
        strcpy(namecopy, argv[i]);         // копия, т.к. basename портит строку
        char *name = basename(namecopy);

        // печатаем строку
        printf("%c%s %ld %s %s %s %s %s\n",
               type, rwx,
               (long)st.st_nlink, // число связей
               owner, group,
               size,
               date,
               name);
    }

    return 0;
}