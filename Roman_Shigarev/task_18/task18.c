#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>

static void make_permissions(mode_t mode, char permissions[])
{
    if (S_ISDIR(mode))
        permissions[0] = 'd';
    else if (S_ISREG(mode))
        permissions[0] = '-';
    else
        permissions[0] = '?';

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

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s path [path...]\n", argv[0]);
        return 1;
    }

    time_t now = time(NULL);
    if (now == (time_t)-1) {
        perror("time");
        return 1;
    }

    int status = 0;

    for (int i = 1; i < argc; i++) {
        struct stat info;

        if (lstat(argv[i], &info) == -1) {
            perror(argv[i]);
            status = 1;
            continue;
        }

        char permissions[11];
        make_permissions(info.st_mode, permissions);

        struct passwd *owner = getpwuid(info.st_uid);
        struct group *group = getgrgid(info.st_gid);

        struct tm *modified = localtime(&info.st_mtime);
        if (modified == NULL) {
            fprintf(stderr, "Cannot convert modification time: %s\n", argv[i]);
            status = 1;
            continue;
        }

        double age = difftime(now, info.st_mtime);
        const char *format = "%b %e %H:%M";

        if (age > 180.0 * 24 * 60 * 60 || age < 0)
            format = "%b %e %Y";

        char date[32];
        if (strftime(date, sizeof(date), format, modified) == 0) {
            fprintf(stderr, "Cannot format modification time: %s\n", argv[i]);
            status = 1;
            continue;
        }

        char size[32] = "";
        if (S_ISREG(info.st_mode))
            snprintf(size, sizeof(size), "%lld", (long long)info.st_size);

        printf("%s %4lu %-16s %-16s %10s %-12s %s\n",
               permissions, (unsigned long)info.st_nlink,
               owner != NULL ? owner->pw_name : "?",
               group != NULL ? group->gr_name : "?",
               size, date, basename(argv[i]));
    }

    return status;
}
