#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>

static void format_mode(mode_t mode, char *str) {
    if (S_ISDIR(mode))
        str[0] = 'd';
    else if (S_ISREG(mode))
        str[0] = '-';
    else if (S_ISLNK(mode))
        str[0] = 'l';
    else
        str[0] = '?';

    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}

static void format_time(time_t mtime, char *buf, size_t bufsize) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&mtime);
    double diff = difftime(now, mtime);

    if (diff > 6.0 * 30 * 24 * 3600)
        strftime(buf, bufsize, "%b %e  %Y", tm_info);
    else
        strftime(buf, bufsize, "%b %e %H:%M", tm_info);
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

        char mode_str[11];
        format_mode(st.st_mode, mode_str);

        struct passwd *pw = getpwuid(st.st_uid);
        const char *owner = pw ? pw->pw_name : "?";

        struct group *gr = getgrgid(st.st_gid);
        const char *group = gr ? gr->gr_name : "?";

        char time_str[64];
        format_time(st.st_mtime, time_str, sizeof(time_str));

        char *name_copy = strdup(argv[i]);
        if (!name_copy) {
            perror("strdup");
            continue;
        }
        const char *name = basename(name_copy);

        if (S_ISREG(st.st_mode))
            printf("%-10s %4ld %-8s %-8s %8lld %s %s\n",
                   mode_str, (long)st.st_nlink, owner, group,
                   (long long)st.st_size, time_str, name);
        else
            printf("%-10s %4ld %-8s %-8s %8s %s %s\n",
                   mode_str, (long)st.st_nlink, owner, group,
                   "", time_str, name);

        free(name_copy);
    }

    return 0;
}