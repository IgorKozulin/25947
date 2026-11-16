#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

void make_mode(mode_t mode, char *s)
{
    // Тип файла.
    if (S_ISDIR(mode))
        s[0] = 'd';
    else if (S_ISREG(mode))
        s[0] = '-';
    else
        s[0] = '?';

    // Права хозяина, группы и остальных.
    s[1] = (mode & S_IRUSR) ? 'r' : '-';
    s[2] = (mode & S_IWUSR) ? 'w' : '-';
    s[3] = (mode & S_IXUSR) ? 'x' : '-';

    s[4] = (mode & S_IRGRP) ? 'r' : '-';
    s[5] = (mode & S_IWGRP) ? 'w' : '-';
    s[6] = (mode & S_IXGRP) ? 'x' : '-';

    s[7] = (mode & S_IROTH) ? 'r' : '-';
    s[8] = (mode & S_IWOTH) ? 'w' : '-';
    s[9] = (mode & S_IXOTH) ? 'x' : '-';

    s[10] = '\0';
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s file...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;
        char mode[11];
        char date[20];
        char size[20] = "";
        const char *name;
        struct passwd *pw;
        struct group *gr;

        // Получаем информацию о файле.
        if (stat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }

        make_mode(st.st_mode, mode);

        // Получаем имя владельца и группы.
        pw = getpwuid(st.st_uid);
        gr = getgrgid(st.st_gid);

        // Размер выводим только для обычного файла.
        if (S_ISREG(st.st_mode))
            snprintf(size, sizeof(size), "%lld",
                     (long long)st.st_size);

        // Форматируем дату последнего изменения.
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M",
                 localtime(&st.st_mtime));

        // Если задан путь, оставляем только имя файла.
        name = strrchr(argv[i], '/');
        name = (name != NULL) ? name + 1 : argv[i];

        printf("%-10s %3lu %-10s %-10s %10s %-16s %s\n",
               mode,
               (unsigned long)st.st_nlink,
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?",
               size,
               date,
               name);
    }

    return 0;
}
