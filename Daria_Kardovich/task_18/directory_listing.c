#include <grp.h>   
#include <pwd.h>  
#include <stdio.h>   
#include <stdlib.h>   
#include <string.h>    
#include <sys/stat.h> 
#include <sys/types.h>
#include <time.h> 
#include <unistd.h>  

//определяет тип файла по его mode
static char file_type(mode_t mode)
{
    // Если это каталог, возвращаем символ d
    if (S_ISDIR(mode)) {
        return 'd';
    }
    // Если это обычный файл, возвращаем символ -
    if (S_ISREG(mode)) {
        return '-';
    }
    // Для остальных типов файлов возвращаем ?
    return '?';
}
// Функция преобразует права доступа файла в строку rwxrwxrwx
static void permissions(mode_t mode, char result[10])
{
    // Проверяем права владельца файла
    result[0] = (mode & S_IRUSR) ? 'r' : '-'; // Владелец может читать
    result[1] = (mode & S_IWUSR) ? 'w' : '-'; // Владелец может изменять
    result[2] = (mode & S_IXUSR) ? 'x' : '-'; // Владелец может запускать

    // Проверяем права группы файла
    result[3] = (mode & S_IRGRP) ? 'r' : '-'; // Группа может читать
    result[4] = (mode & S_IWGRP) ? 'w' : '-'; // Группа может изменять
    result[5] = (mode & S_IXGRP) ? 'x' : '-'; // Группа может запускать

    // Проверяем права остальных пользователей
    result[6] = (mode & S_IROTH) ? 'r' : '-'; // Остальные могут читать
    result[7] = (mode & S_IWOTH) ? 'w' : '-'; // Остальные могут изменять
    result[8] = (mode & S_IXOTH) ? 'x' : '-'; // Остальные могут запускать
    
    result[9] = '\0';
}

// Функция возвращает только имя файла без пути к нему
static const char *basename_from_path(const char *path)
{
    // Ищем последний символ / в пути
    const char *last_slash = strrchr(path, '/');

    // Если / нет, значит path уже содержит только имя файла
    if (last_slash == NULL) {
        return path;
    }

    // Возвращаем адрес символа после последнего /
    return last_slash + 1;
}

// Функция выводит всю информацию об одном файле или каталоге
static void print_file_info(const char *path)
{
    // Структура для информации о файле
    struct stat info;

    // Указатель на информацию о владельце
    struct passwd *owner;

    // Указатель на информацию о группе
    struct group *group;

    // Строка с правами, например rw-r--r--
    char access[10];

    // Строка с датой изменения файла
    char date[32];

    // Строка с размером файла
    char size[32];

    // Структура с разобранной датой
    struct tm *time_info;

    // lstat получает информацию о файле и записывает её в info
    if (lstat(path, &info) == -1) {
        // Если файла нет или нет доступа, выводим ошибку
        perror(path);
        return;
    }

    // Преобразуем права из info.st_mode в текстовую строку
    permissions(info.st_mode, access);

    // По числовому идентификатору владельца находим его имя
    owner = getpwuid(info.st_uid);

    // По числовому идентификатору группы находим её имя
    group = getgrgid(info.st_gid);

    // Преобразуем время изменения файла в структуру даты
    time_info = localtime(&info.st_mtime);

    // Если дата успешно получена.
    if (time_info != NULL) {
        // Преобразуем дату в текст, например Oct 13 14:20.
        strftime(date, sizeof(date), "%b %d %H:%M", time_info);
    } else {
        // Если дату получить не удалось
        strcpy(date, "unknown");
    }

    // Размер нужен только для обычного файла
    if (S_ISREG(info.st_mode)) {
        // Превращаем размер в байтах в строку
        snprintf(size, sizeof(size), "%lld", (long long)info.st_size);
    } else {
        // Для каталога оставляем поле размера пустым
        strcpy(size, "");
    }

    // Выводим одну строку в формате, похожем на ls -ld
    printf("%c%s %3lu %-12s %-12s %8s %-12s %s\n",
           file_type(info.st_mode),                    // Тип: d, - или ?
           access,                                     // Права доступа
           (unsigned long)info.st_nlink,               // Количество ссылок
           owner != NULL ? owner->pw_name : "unknown", // Имя владельца
           group != NULL ? group->gr_name : "unknown", // Имя группы
           size,                                       // Размер обычного файла
           date,                                       // Дата изменения
           basename_from_path(path));                  // Имя файла без пути
}

// Главная функция программы
int main(int argc, char *argv[])
{
    // Если пользователь не передал аргументы
    if (argc == 1) {
        // Выводим информацию о текущем каталоге
        print_file_info(".");
        return 0;
    }

    // Обрабатываем каждый файл или каталог из аргументов
    for (int i = 1; i < argc; i++) {
        // Выводим информацию о текущем аргументе
        print_file_info(argv[i]);
    }
    return 0;
}
