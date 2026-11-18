#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <libgen.h>

static void print_perms(mode_t mode)
{
    char type;
    if (S_ISDIR(mode)) type = 'd';
    else if (S_ISREG(mode)) type = '-';
    else if (S_ISLNK(mode)) type = 'l';
    else type = '?';

    char p[11];
    p[0] = type;
    p[1] = (mode & S_IRUSR) ? 'r' : '-';
    p[2] = (mode & S_IWUSR) ? 'w' : '-';
    p[3] = (mode & S_IXUSR) ? 'x' : '-';
    p[4] = (mode & S_IRGRP) ? 'r' : '-';
    p[5] = (mode & S_IWGRP) ? 'w' : '-';
    p[6] = (mode & S_IXGRP) ? 'x' : '-';
    p[7] = (mode & S_IROTH) ? 'r' : '-';
    p[8] = (mode & S_IWOTH) ? 'w' : '-';
    p[9] = (mode & S_IXOTH) ? 'x' : '-';
    p[10] = '\0';
    printf("%s", p);
}

int main(int argc, char *argv[])
{
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

        print_perms(st.st_mode);
        printf(" %3ld", (long)st.st_nlink);

        struct passwd *pw = getpwuid(st.st_uid);
        const char *uname = pw ? pw->pw_name : "?";
        printf(" %-8s", uname);

        struct group *gr = getgrgid(st.st_gid);
        const char *gname = gr ? gr->gr_name : "?";
        printf(" %-8s", gname);

        if (S_ISREG(st.st_mode))
            printf(" %8lld", (long long)st.st_size);
        else
            printf(" %8s", "");

        char tbuf[64];
        time_t now = time(NULL);
        struct tm *tm = localtime(&st.st_mtime);
        if (now - st.st_mtime > 15552000)
            strftime(tbuf, sizeof(tbuf), "%b %e  %Y", tm);
        else
            strftime(tbuf, sizeof(tbuf), "%b %e %H:%M", tm);
        printf(" %s", tbuf);

        char *path_copy = strdup(argv[i]);
        printf(" %s\n", basename(path_copy));
        free(path_copy);
    }

    return 0;
}
