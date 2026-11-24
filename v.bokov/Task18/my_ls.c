#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <libgen.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Enter filename\n");
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        struct stat st;

        if (lstat(argv[i], &st) == -1)
        {
            perror(argv[i]);
            continue;
        }

        char mode[11];

        if (S_ISDIR(st.st_mode))
            mode[0] = 'd';
        else if (S_ISREG(st.st_mode))
            mode[0] = '-';
        else
            mode[0] = '?';

        mode[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        mode[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        mode[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';

        mode[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        mode[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        mode[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';

        mode[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        mode[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        mode[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';

        mode[10] = '\0';

        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);

        const char *owner = pw ? pw->pw_name : "?";
        const char *group = gr ? gr->gr_name : "?";

        char size[20] = "";

        if (S_ISREG(st.st_mode))
            snprintf(size, sizeof(size), "%lld",
                     (long long)st.st_size);

        char date[30];

        time_t now = time(NULL);
        struct tm *tm = localtime(&st.st_mtime);

        if (tm != NULL)
        {
            if (st.st_mtime > now ||
                now - st.st_mtime > 180L * 24 * 60 * 60)
            {
                strftime(date, sizeof(date),
                         "%b %e %Y", tm);
            }
            else
            {
                strftime(date, sizeof(date),
                         "%b %e %H:%M", tm);
            }
        }
        else
        {
            strcpy(date, "?");
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s", argv[i]);

        printf("%-10s %4lu %-10s %-10s %10s %-12s %s\n",
               mode,
               (unsigned long)st.st_nlink,
               owner,
               group,
               size,
               date,
               basename(path));
    }

    return 0;
}