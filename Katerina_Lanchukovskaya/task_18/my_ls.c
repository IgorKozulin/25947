
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> // права доступа
#include <unistd.h>
#include <pwd.h>  // получение имени владельца
#include <grp.h>  // получение имени группы
#include <time.h>
#include <string.h>
#include <libgen.h>  // имя файла

void get_permissions(mode_t mode, char *permissions){
    permissions[0] = S_ISDIR(mode) ? 'd' : (S_ISREG(mode) ? '-' : '?');

    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';  // бит разрешения владельца
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';

    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';

    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';

    permissions[10] = '\0';
}

void get_date(time_t modification_time, char *date){
    time_t current_time = time(NULL);  // текущее время
    struct tm *tm_info = localtime(&modification_time);

    if (current_time - modification_time > 15552000){
        strftime(date, 32, "%b %e %Y", tm_info);
    }
    else{
        strftime(date, 32, "%b %e %H:%M", tm_info);
    }
}

void print_file_info(char *path){
    struct stat st;

    if (lstat(path, &st) == -1){  // получает информацию о файле
        perror(path);
        return;
    }

    char permissions[11];
    get_permissions(st.st_mode, permissions);

    struct passwd *user = getpwuid(st.st_uid);
    struct group *group = getgrgid(st.st_gid);

    const char *owner_name = user ? user->pw_name : "?";
    const char *group_name = group ? group->gr_name : "?";

    char size[32] = "";

    if (S_ISREG(st.st_mode)){
        snprintf(size, sizeof(size), "%lld", (long long)st.st_size);
    }

    char date[32];
    get_date(st.st_mtime, date);

    char filename[4096];
    snprintf(filename, sizeof(filename), "%s", path);

    printf("%-10s %4lu %-10s %-10s %10s %-12s %s\n",
           permissions,
           (unsigned long)st.st_nlink,
           owner_name,
           group_name,
           size,
           date,
           basename(filename));
}

int main(int argc, char **argv){
    if (argc < 2){
        printf("Usage: %s file1 [file2 ...]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++){
        print_file_info(argv[i]);
    }

    return 0;
}

