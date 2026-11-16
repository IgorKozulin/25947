
#define _POSIX_C_SOURCE 200809L

#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static void format_mode(mode_t mode, char text[11])
{
    static const mode_t bits[] = {
        S_IRUSR, S_IWUSR, S_IXUSR,
        S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };
    static const char rights[] = "rwxrwxrwx";

    text[0] = '?';
    if (S_ISDIR(mode)) {
        text[0] = 'd';
    } else if (S_ISREG(mode)) {
        text[0] = '-';
    }

    for (size_t i = 0; i < sizeof(bits) / sizeof(bits[0]); ++i) {
        text[i + 1] = mode & bits[i] ? rights[i] : '-';
    }
    text[10] = '\0';
}

static void print_name(const char *path)
{
    const char *end = path + strlen(path);
    while (end > path + 1 && end[-1] == '/') {
        --end;
    }

    const char *name = end;
    while (name > path && name[-1] != '/') {
        --name;
    }
    if (name == end) {
        name = path;
    }

    fwrite(name, 1, end - name, stdout);
    putchar('\n');
}

static int list_path(const char *path)
{
    struct stat info;
    if (lstat(path, &info) == -1) {
        perror(path);
        return EXIT_FAILURE;
    }

    char mode[11];
    format_mode(info.st_mode, mode);

    char owner_id[32];
    char group_id[32];
    struct passwd *owner = getpwuid(info.st_uid);
    struct group *group = getgrgid(info.st_gid);
    snprintf(owner_id, sizeof(owner_id), "%" PRIuMAX, (uintmax_t)info.st_uid);
    snprintf(group_id, sizeof(group_id), "%" PRIuMAX, (uintmax_t)info.st_gid);

    char size[32] = "";
    if (S_ISREG(info.st_mode)) {
        snprintf(size, sizeof(size), "%" PRIdMAX, (intmax_t)info.st_size);
    }

    char date[32];
    struct tm local;
    if (localtime_r(&info.st_mtime, &local) == NULL ||
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &local) == 0) {
        strcpy(date, "?");
    }

    printf("%s %4" PRIuMAX " %-12s %-12s %12s %-19s ",
           mode, (uintmax_t)info.st_nlink,
           owner ? owner->pw_name : owner_id,
           group ? group->gr_name : group_id, size, date);
    print_name(path);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        return list_path(".");
    }

    int result = EXIT_SUCCESS;
    for (int i = 1; i < argc; ++i) {
        if (list_path(argv[i]) != EXIT_SUCCESS) {
            result = EXIT_FAILURE;
        }
    }
    return result;
}
