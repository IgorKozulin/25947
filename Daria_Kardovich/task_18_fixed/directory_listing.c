#define _POSIX_C_SOURCE 200809L

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

static char file_type(mode_t mode)
{
    if (S_ISDIR(mode)) {
        return 'd';
    }

    if (S_ISREG(mode)) {
        return '-';
    }

    return '?';
}

static void permissions(mode_t mode, char result[10])
{
    const mode_t bits[9] = {
        S_IRUSR, S_IWUSR, S_IXUSR,
        S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };

    const char symbols[] = "rwxrwxrwx";

    for (int i = 0; i < 9; ++i) {
        result[i] = (mode & bits[i]) ? symbols[i] : '-';
    }

    result[9] = '\0';
}

static void print_name(const char *path)
{
    size_t length = strlen(path);
    size_t start;

    while (length > 1 && path[length - 1] == '/') {
        --length;
    }

    start = length;

    while (start > 0 && path[start - 1] != '/') {
        --start;
    }

    if (start == length && length == 1 && path[0] == '/') {
        putchar('/');
    } else {
        fwrite(path + start, 1, length - start, stdout);
    }
}

static int print_file_info(const char *path)
{
    struct stat info;
    struct passwd *owner;
    struct group *group;
    struct tm *time_info;

    char access[10];
    char date[32];
    char size[32];
    char owner_id[32];
    char group_id[32];

    if (lstat(path, &info) == -1) {
        perror(path);
        return 1;
    }

    permissions(info.st_mode, access);

    owner = getpwuid(info.st_uid);
    group = getgrgid(info.st_gid);

    snprintf(owner_id, sizeof owner_id, "%lu", (unsigned long)info.st_uid);
    snprintf(group_id, sizeof group_id, "%lu", (unsigned long)info.st_gid);

    time_info = localtime(&info.st_mtime);

    if (time_info == NULL ||
        strftime(date, sizeof date, "%Y-%m-%d %H:%M", time_info) == 0) {
        strcpy(date, "unknown");
    }

    if (S_ISREG(info.st_mode)) {
        snprintf(size, sizeof size, "%lld", (long long)info.st_size);
    } else {
        size[0] = '\0';
    }

    printf("%c%s %3lu %-12s %-12s %10s %-16s ",
           file_type(info.st_mode),
           access,
           (unsigned long)info.st_nlink,
           owner ? owner->pw_name : owner_id,
           group ? group->gr_name : group_id,
           size,
           date);

    print_name(path);
    putchar('\n');

    return 0;
}

int main(int argc, char *argv[])
{
    int error = 0;

    if (argc == 1) {
        return print_file_info(".");
    }

    for (int i = 1; i < argc; ++i) {
        if (print_file_info(argv[i]) != 0) {
            error = 1;
        }
    }

    return error;
}
