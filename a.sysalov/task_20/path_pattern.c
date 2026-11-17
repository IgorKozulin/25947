#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int matches(const char *pattern, const char *name)
{
    const char *star = NULL;
    const char *retry = NULL;
    while (*name != '\0') {
        if (*pattern == '*') {
            star = pattern++;
            retry = name;
        } else if (*pattern == '?' || *pattern == *name) {
            pattern++;
            name++;
        } else if (star != NULL) {
            pattern = star + 1;
            name = ++retry;
        } else {
            return 0;
        }
    }
    while (*pattern == '*') pattern++;
    return *pattern == '\0';
}

static int found = 0;
static int status = 0;

void search(const char *prefix, const char *pattern)
{
    if (*pattern == '\0') {
        struct stat info;
        if (stat(*prefix ? prefix : ".", &info) == 0 && S_ISDIR(info.st_mode)) {
            puts(*prefix ? prefix : ".");
            found = 1;
        }
        return;
    }
    const char *slash = strchr(pattern, '/');
    size_t length = slash ? (size_t)(slash - pattern) : strlen(pattern);
    char *component = strndup(pattern, length);
    if (component == NULL) {
        perror("strndup");
        status = 1;
        return;
    }
    DIR *directory = opendir(*prefix ? prefix : ".");
    if (directory == NULL) {
        if (errno != ENOENT && errno != ENOTDIR) {
            perror(*prefix ? prefix : ".");
            status = 1;
        }
        free(component);
        return;
    }
    for (;;) {
        errno = 0;
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                perror("readdir");
                status = 1;
            }
            break;
        }
        if ((!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                && strcmp(component, entry->d_name) != 0) continue;
        if (!matches(component, entry->d_name)) continue;
        size_t size = strlen(prefix) + strlen(entry->d_name) + 2;
        char *path = malloc(size);
        if (path == NULL) {
            perror("malloc");
            status = 1;
            break;
        }
        snprintf(path, size, "%s%s%s", prefix, entry->d_name, slash ? "/" : "");
        if (slash) {
            const char *remaining = slash + 1;
            while (*remaining == '/') remaining++;
            search(path, remaining);
        } else {
            puts(path);
            found = 1;
        }
        free(path);
    }
    closedir(directory);
    free(component);
}

int main(void)
{
    char *pattern = NULL;
    size_t capacity = 0;
    printf("Enter pathname pattern: ");
    fflush(stdout);
    ssize_t length = getline(&pattern, &capacity, stdin);
    if (length == -1) {
        free(pattern);
        return 1;
    }
    if (length > 0 && pattern[length - 1] == '\n') pattern[--length] = '\0';
    const char *remaining = pattern;
    if (*remaining == '/') {
        while (*remaining == '/') remaining++;
        search("/", remaining);
    } else if (*remaining != '\0') {
        search("", remaining);
    }
    if (!found) puts(pattern);
    free(pattern);
    return status;
}
