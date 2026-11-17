#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <libgen.h>

/* Формирование строки прав доступа (10 символов) */
void print_mode(mode_t mode) {
    char str[11];
    
    /* 1. Тип файла */
    if (S_ISDIR(mode))       str[0] = 'd';
    else if (S_ISREG(mode))  str[0] = '-';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else                     str[0] = '?';

    /* 2. Права владельца */
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* 3. Права группы */
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* 4. Права остальных */
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    str[10] = '\0';
    printf("%s", str);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;
        
        /* Получаем информацию о файле. lstat не разыменовывает ссылки */
        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }

        /* 1. Права доступа */
        print_mode(st.st_mode);
        
        /* 2. Количество связей */
        printf(" %3lu", (unsigned long)st.st_nlink);

        /* 3. Владелец и группа */
        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        printf(" %-8s %-8s", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

        /* 4. Размер (только для обычных файлов) */
        if (S_ISREG(st.st_mode)) {
            printf(" %8lld", (long long)st.st_size);
        } else {
            printf(" %8s", ""); /* Пустое поле для каталогов/устройств */
        }

        /* 5. Дата модификации */
        time_t now = time(NULL);
        struct tm *tm = localtime(&st.st_mtime);
        char datebuf[64];
        
        /* Если файл старше 6 месяцев (или из будущего) — показываем год */
        if (now - st.st_mtime > 15552000 || st.st_mtime - now > 0) {
            strftime(datebuf, sizeof(datebuf), "%b %e  %Y", tm);
        } else {
            strftime(datebuf, sizeof(datebuf), "%b %e %H:%M", tm);
        }
        printf(" %s", datebuf);

        /* 6. Имя файла (только базовое имя без пути) */
        char *path_copy = strdup(argv[i]);
        char *base = basename(path_copy);
        printf(" %s\n", base);
        free(path_copy);
    }

    return 0;
}
