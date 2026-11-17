
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void get_permissions(mode_t mode, char *permissions)
{
    permissions[0] = S_ISDIR(mode) ? 'd' : (S_ISREG(mode) ? '-' : '?');

    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
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

const char *get_filename(const char *path)
{
    const char *slash = strrchr(path, '/');

    if (slash == NULL) {
        return path;
    }

    return slash + 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return 1;
    }

    int result = 0;

    for (int i = 1; i < argc; i++) {
        struct stat file_stat;

        if (lstat(argv[i], &file_stat) == -1) {
            perror(argv[i]);
            result = 1;
            continue;
        }

        char permissions[11];
        get_permissions(file_stat.st_mode, permissions);

        struct passwd *owner = getpwuid(file_stat.st_uid);
        struct group *group = getgrgid(file_stat.st_gid);

        char owner_name[32];
        char group_name[32];

        if (owner != NULL) {
            snprintf(owner_name, sizeof(owner_name), "%s", owner->pw_name);
        } else {
            snprintf(owner_name, sizeof(owner_name), "%u",
                     (unsigned int)file_stat.st_uid);
        }

        if (group != NULL) {
            snprintf(group_name, sizeof(group_name), "%s", group->gr_name);
        } else {
            snprintf(group_name, sizeof(group_name), "%u",
                     (unsigned int)file_stat.st_gid);
        }

        char size[32] = "";

        if (S_ISREG(file_stat.st_mode)) {
            snprintf(size, sizeof(size), "%lld",
                     (long long)file_stat.st_size);
        }

        char date[32];
        struct tm *time_info = localtime(&file_stat.st_mtime);

        if (time_info == NULL ||
            strftime(date, sizeof(date), "%b %d %H:%M", time_info) == 0) {
            snprintf(date, sizeof(date), "?");
        }

        printf("%-10s %3lu %-12s %-12s %10s %-15s %s\n",
               permissions,
               (unsigned long)file_stat.st_nlink,
               owner_name,
               group_name,
               size,
               date,
               get_filename(argv[i]));
    }

    return result;
}

