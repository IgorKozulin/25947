#define _POSIX_C_SOURCE 200809L
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
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
        char permissions[11] = "----------";
        permissions[0] = S_ISDIR(info.st_mode) ? 'd'
                         : S_ISREG(info.st_mode) ? '-' : '?';
        const mode_t bits[] = {S_IRUSR, S_IWUSR, S_IXUSR,
                               S_IRGRP, S_IWGRP, S_IXGRP,
                               S_IROTH, S_IWOTH, S_IXOTH};
        for (int j = 0; j < 9; j++) {
            if (info.st_mode & bits[j]) permissions[j + 1] = "rwx"[j % 3];
        }
        char owner[256];
        char group[256];
        struct passwd *user = getpwuid(info.st_uid);
        if (user) snprintf(owner, sizeof(owner), "%s", user->pw_name);
        else snprintf(owner, sizeof(owner), "%lu", (unsigned long)info.st_uid);
        struct group *entry = getgrgid(info.st_gid);
        if (entry) snprintf(group, sizeof(group), "%s", entry->gr_name);
        else snprintf(group, sizeof(group), "%lu", (unsigned long)info.st_gid);

        char size[32] = "";
        if (S_ISREG(info.st_mode) || S_ISDIR(info.st_mode)) {
            snprintf(size, sizeof(size), "%lld", (long long)info.st_size);
        }
        char date[32] = "?";
        struct tm *modified = localtime(&info.st_mtime);
        if (modified) strftime(date, sizeof(date), "%Y-%m-%d %H:%M", modified);
        printf("%s %3lu %-12s %-12s %10s %s %s\n",
               permissions, (unsigned long)info.st_nlink, owner, group,
               size, date, basename(argv[i]));
    }
    return status;
}
