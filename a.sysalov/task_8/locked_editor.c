#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int mandatory = argc == 3 && strcmp(argv[1], "--mandatory") == 0;
    if (argc != 2 && !mandatory) {
        fprintf(stderr, "Usage: %s [--mandatory] file\n", argv[0]);
        return 1;
    }

    const char *filename = argv[argc - 1];
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct stat info;
    if (fstat(fd, &info) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }
    if (mandatory && fchmod(fd, (info.st_mode | S_ISGID) & ~S_IXGRP) == -1) {
        perror("fchmod");
        close(fd);
        return 1;
    }

    struct flock lock = {0};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("fcntl: file is locked");
        if (mandatory) {
            fchmod(fd, info.st_mode & 07777);
        }
        close(fd);
        return 1;
    }
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        perror("fcntl");
        if (mandatory) fchmod(fd, info.st_mode & 07777);
        close(fd);
        return 1;
    }

    const char *editor = getenv("EDITOR");
    if (editor == NULL || *editor == '\0') {
        editor = "vi";
    }
    char *command = malloc(strlen(editor) + 4 * strlen(filename) + 16);
    if (command == NULL) {
        perror("malloc");
        if (mandatory) {
            fchmod(fd, info.st_mode & 07777);
        }
        close(fd);
        return 1;
    }
    char *out = command + sprintf(command, "%s './", editor);
    if (filename[0] == '/') {
        out -= 2;
    }
    for (const char *p = filename; *p != '\0'; p++) {
        if (*p == '\'') {
            memcpy(out, "'\\''", 4);
            out += 4;
        } else {
            *out++ = *p;
        }
    }
    *out++ = '\'';
    *out = '\0';
    printf("File locked. Starting editor.\n");
    fflush(stdout);
    int result = system(command);
    free(command);

    int status = result == -1 || !WIFEXITED(result) || WEXITSTATUS(result) != 0;
    if (mandatory && fchmod(fd, info.st_mode & 07777) == -1) {
        perror("fchmod");
        status = 1;
    }
    close(fd);
    return status;
}
