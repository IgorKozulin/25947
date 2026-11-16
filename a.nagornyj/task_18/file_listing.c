#define _POSIX_C_SOURCE 200809L

#include <grp.h>
#include <inttypes.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static void
make_permissions(mode_t mode, char permissions[11])
{
    permissions[0] = S_ISDIR(mode) ? 'd' : S_ISREG(mode) ? '-' : '?';
    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';
    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';
    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';
    if (mode & S_ISUID) {
        permissions[3] = (mode & S_IXUSR) ? 's' : 'S';
    }
    if (mode & S_ISGID) {
        permissions[6] = (mode & S_IXGRP) ? 's' : 'S';
    }
    if (mode & S_ISVTX) {
        permissions[9] = (mode & S_IXOTH) ? 't' : 'T';
    }
    permissions[10] = '\0';
}

static void
make_date(time_t modified, char output[32])
{
    time_t now = time(NULL);
    struct tm current;
    struct tm cutoff_tm;
    time_t cutoff;
    const char *format = "%b %e %H:%M";

    if (now != (time_t)-1 && localtime_r(&now, &current) != NULL) {
        cutoff_tm = current;
        cutoff_tm.tm_mon -= 6;
        cutoff = mktime(&cutoff_tm);
        if (cutoff != (time_t)-1 && modified < cutoff) {
            format = "%b %e %Y";
        }
    }
    if (localtime_r(&modified, &current) == NULL ||
        strftime(output, 32, format, &current) == 0) {
        (void)snprintf(output, 32, "unknown");
    }
}

static int
print_path(const char *path)
{
    struct stat info;
    char permissions[11];
    char owner[64];
    char group[64];
    char size[32] = "";
    char date[32];
    char *path_copy;
    char *name;
    struct passwd *owner_info;
    struct group *group_info;

    if (lstat(path, &info) == -1) {
        perror(path);
        return -1;
    }
    make_permissions(info.st_mode, permissions);

    owner_info = getpwuid(info.st_uid);
    if (owner_info != NULL) {
        (void)snprintf(owner, sizeof(owner), "%s", owner_info->pw_name);
    } else {
        (void)snprintf(owner, sizeof(owner), "%lu",
            (unsigned long)info.st_uid);
    }
    group_info = getgrgid(info.st_gid);
    if (group_info != NULL) {
        (void)snprintf(group, sizeof(group), "%s", group_info->gr_name);
    } else {
        (void)snprintf(group, sizeof(group), "%lu",
            (unsigned long)info.st_gid);
    }
    if (S_ISREG(info.st_mode)) {
        (void)snprintf(size, sizeof(size), "%" PRIdMAX,
            (intmax_t)info.st_size);
    }
    make_date(info.st_mtime, date);

    path_copy = strdup(path);
    if (path_copy == NULL) {
        perror("strdup");
        return -1;
    }
    name = basename(path_copy);
    printf("%-10s %4lu %-12s %-12s %10s %-14s %s\n",
        permissions, (unsigned long)info.st_nlink, owner, group, size,
        date, name != NULL ? name : path);
    free(path_copy);
    return 0;
}

int
main(int argc, char **argv)
{
    int status = EXIT_SUCCESS;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s path [path ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    printf("%-10s %4s %-12s %-12s %10s %-14s %s\n",
        "MODE", "LINKS", "OWNER", "GROUP", "SIZE", "MODIFIED", "NAME");
    for (int index = 1; index < argc; ++index) {
        if (print_path(argv[index]) == -1) {
            status = EXIT_FAILURE;
        }
    }
    return status;
}
