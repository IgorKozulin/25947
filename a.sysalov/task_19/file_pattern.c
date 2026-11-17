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

int main(void)
{
    char *pattern = NULL;
    size_t capacity = 0;
    printf("Enter filename pattern: ");
    fflush(stdout);
    ssize_t length = getline(&pattern, &capacity, stdin);
    if (length == -1) {
        free(pattern);
        return 1;
    }
    if (length > 0 && pattern[length - 1] == '\n') pattern[--length] = '\0';
    if (strchr(pattern, '/') != NULL) {
        fprintf(stderr, "The pattern must not contain '/'.\n");
        free(pattern);
        return 1;
    }
    DIR *directory = opendir(".");
    if (directory == NULL) {
        perror("opendir");
        free(pattern);
        return 1;
    }
    int found = 0;
    int status = 0;
    struct dirent *entry;
    for (;;) {
        errno = 0;
        entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                perror("readdir");
                status = 1;
            }
            break;
        }
        if ((!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                && strcmp(pattern, entry->d_name) != 0) continue;
        if (matches(pattern, entry->d_name)) {
            puts(entry->d_name);
            found = 1;
        }
    }
    if (!found) puts(pattern);
    closedir(directory);
    free(pattern);
    return status;
}
