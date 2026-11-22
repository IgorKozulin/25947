#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <libgen.h>

void print_mode(mode_t mode) {
    char str[11];
    if (S_ISDIR(mode))       str[0] = 'd';
    else if (S_ISREG(mode))  str[0] = '-';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else                     str[0] = '?';
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
    printf("%s", str);
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
        print_mode(st.st_mode);
        printf(" %3lu", (unsigned long)st.st_nlink);
        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        printf(" %-8s %-8s", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");
        
        // ВОТ ЗДЕСЬ МЫ УБРАЛИ ПРОВЕРКУ. Теперь размер выводится всегда.
        printf(" %8lld", (long long)st.st_size);
        
        time_t now = time(NULL);
        struct tm *tm = localtime(&st.st_mtime);
        char datebuf[64];
        if (now - st.st_mtime > 15552000 || st.st_mtime - now > 0) {
            strftime(datebuf, sizeof(datebuf), "%b %e  %Y", tm);
        } else {
            strftime(datebuf, sizeof(datebuf), "%b %e %H:%M", tm);
        }
        printf(" %s", datebuf);
        char *path_copy = strdup(argv[i]);
        char *base = basename(path_copy);
        printf(" %s\n", base);
        free(path_copy);
    }
    return 0;
}
