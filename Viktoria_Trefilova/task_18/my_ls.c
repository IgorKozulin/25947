#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>

int main(int argc, char *argv[])
{
    int result = 0;
    const mode_t bits[] = {
        S_IRUSR, S_IWUSR, S_IXUSR,
        S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };

    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;
        char rights[11] = "----------";
        char date[32];
        char size[32] = "";
        struct passwd *owner;
        struct group *group;
        struct tm *modified;

        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            result = 1;
            continue;
        }

        rights[0] = S_ISDIR(st.st_mode) ? 'd' :
                    S_ISREG(st.st_mode) ? '-' : '?';

        for (int j = 0; j < 9; j++)
            if (st.st_mode & bits[j])
                rights[j + 1] = "rwx"[j % 3];

        owner = getpwuid(st.st_uid);
        group = getgrgid(st.st_gid);

        if (S_ISREG(st.st_mode))
            snprintf(size, sizeof(size), "%lld", (long long)st.st_size);

        modified = localtime(&st.st_mtime);
        if (modified == NULL)
            snprintf(date, sizeof(date), "?");
        else
            strftime(date, sizeof(date), "%Y-%m-%d %H:%M", modified);

        printf("%s %4lu %-16s %-16s %12s %s %s\n",
               rights, (unsigned long)st.st_nlink,
               owner ? owner->pw_name : "?",
               group ? group->gr_name : "?",
               size, date, basename(argv[i]));
    }

    return result;
}
