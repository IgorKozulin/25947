#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>

int main(int argc, char *argv[])
{
    int error = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s file [file...]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;
        char permissions[11] = "?---------";
        char date[32];
        struct passwd *owner;
        struct group *group;
        struct tm *modified;

        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            error = 1;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            permissions[0] = 'd';
        } else if (S_ISREG(st.st_mode)) {
            permissions[0] = '-';
        }

        permissions[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        permissions[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        permissions[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        permissions[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        permissions[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        permissions[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
        permissions[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        permissions[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        permissions[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';

        owner = getpwuid(st.st_uid);
        group = getgrgid(st.st_gid);
        modified = localtime(&st.st_mtime);

        if (modified == NULL) {
            perror("localtime");
            error = 1;
            continue;
        }

        if (difftime(time(NULL), st.st_mtime) > 180.0 * 24 * 60 * 60) {
            strftime(date, sizeof(date), "%b %e %Y", modified);
        } else {
            strftime(date, sizeof(date), "%b %e %H:%M", modified);
        }

        printf("%s %4lu %-12s %-12s ", permissions,
               (unsigned long)st.st_nlink,
               owner ? owner->pw_name : "?",
               group ? group->gr_name : "?");

        if (S_ISREG(st.st_mode)) {
            printf("%10lld ", (long long)st.st_size);
        } else {
            printf("%10s ", "");
        }

        printf("%-12s %s\n", date, basename(argv[i]));
    }

    return error;
}
