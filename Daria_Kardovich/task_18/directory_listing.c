#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

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
    result[0] = (mode & S_IRUSR) ? 'r' : '-';
    result[1] = (mode & S_IWUSR) ? 'w' : '-';
    result[2] = (mode & S_IXUSR) ? 'x' : '-';

    result[3] = (mode & S_IRGRP) ? 'r' : '-';
    result[4] = (mode & S_IWGRP) ? 'w' : '-';
    result[5] = (mode & S_IXGRP) ? 'x' : '-';

    result[6] = (mode & S_IROTH) ? 'r' : '-';
    result[7] = (mode & S_IWOTH) ? 'w' : '-';
    result[8] = (mode & S_IXOTH) ? 'x' : '-';
    result[9] = '\0';
}

static const char *basename_from_path(const char *path)
{
    const char *last_slash = strrchr(path, '/');

    if (last_slash == NULL) {
        return path;
    }

    return last_slash + 1;
}

static void print_file_info(const char *path)
{
    struct stat info;
    struct passwd *owner;
    struct group *group;
    char access[10];
    char date[32];
    char size[32];
    struct tm *time_info;

    if (lstat(path, &info) == -1) {
        perror(path);
        return;
    }

    permissions(info.st_mode, access);

    owner = getpwuid(info.st_uid);
    group = getgrgid(info.st_gid);
    time_info = localtime(&info.st_mtime);

    if (time_info != NULL) {
        strftime(date, sizeof(date), "%b %d %H:%M", time_info);
    } else {
        strcpy(date, "unknown");
    }

    if (S_ISREG(info.st_mode)) {
        snprintf(size, sizeof(size), "%lld", (long long)info.st_size);
    } else {
        strcpy(size, "");
    }

    printf("%c%s %3lu %-12s %-12s %8s %-12s %s\n",
           file_type(info.st_mode),
           access,
           (unsigned long)info.st_nlink,
           owner != NULL ? owner->pw_name : "unknown",
           group != NULL ? group->gr_name : "unknown",
           size,
           date,
           basename_from_path(path));
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        print_file_info(".");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        print_file_info(argv[i]);
    }

    return 0;
}
