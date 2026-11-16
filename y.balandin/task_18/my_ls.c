#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>

/* Формирование строки прав доступа (10 символов: тип + 9 бит rwx) */
static void format_mode(mode_t mode, char *out) {
    /* Тип файла */
    if (S_ISDIR(mode))       out[0] = 'd';
    else if (S_ISREG(mode))  out[0] = '-';
    else                     out[0] = '?';  /* всё остальное, кроме dir и regular */

    /* Права владельца */
    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* Права группы */
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* Права остальных */
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';

    out[10] = '\0';
}

/* Форматирование даты модификации: свежие файлы — время, старые — год */
static void format_time(time_t mtime, char *out, size_t out_size) {
    struct tm *tm_info;
    time_t now = time(NULL);
    /* Разница больше ~6 месяцев (примерно 15552000 секунд) */
    int recent = (now - mtime) < (time_t)(6L * 30L * 24L * 60L * 60L)
                 && mtime <= now;

    tm_info = localtime(&mtime);
    if (tm_info == NULL) {
        snprintf(out, out_size, "?");
        return;
    }

    if (recent)
        strftime(out, out_size, "%b %e %H:%M", tm_info);
    else
        strftime(out, out_size, "%b %e  %Y", tm_info);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;

        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }

        /* 1. Права доступа */
        char mode_str[11];
        format_mode(st.st_mode, mode_str);

        /* 2. Имя владельца */
        char *owner = "?";
        struct passwd *pw = getpwuid(st.st_uid);
        if (pw != NULL)
            owner = pw->pw_name;

        /* 3. Имя группы */
        char *group = "?";
        struct group *gr = getgrgid(st.st_gid);
        if (gr != NULL)
            group = gr->gr_name;

        /* 4. Размер: только для обычных файлов */
        char size_str[32];
        if (S_ISREG(st.st_mode))
            snprintf(size_str, sizeof(size_str), "%lld", (long long)st.st_size);
        else
            size_str[0] = '\0';

        /* 5. Дата модификации */
        char time_str[64];
        format_time(st.st_mtime, time_str, sizeof(time_str));

        /* 6. Имя файла без пути */
        /* basename() может модифицировать строку — делаем копию */
        char path_copy[4096];
        strncpy(path_copy, argv[i], sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';
        char *name = basename(path_copy);

        /* 7. Вывод в табличном виде */
        printf("%-10s %3ld %-8s %-8s %8s %12s %s\n",
               mode_str,
               (long)st.st_nlink,
               owner,
               group,
               size_str,
               time_str,
               name);
    }

    return 0;
}
