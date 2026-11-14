#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

extern char **environ;

int my_execvpe(const char *file, char *const argv[], char *const envp[]) {
    if (strchr(file, '/') != NULL) {
        execve(file, argv, envp);
        return -1;
    }

    const char *path_env = getenv("PATH");
    if (path_env == NULL) path_env = "/bin:/usr/bin";

    char *path_copy = strdup(path_env);
    if (path_copy == NULL) return -1;

    char *dir = strtok(path_copy, ":");
    while (dir != NULL) {
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, file);
        execve(full_path, argv, envp);
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    errno = ENOENT;
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return 1;
    }
    printf("[TEST] Running: %s\n", argv[1]);
    my_execvpe(argv[1], &argv[1], environ);
    perror("my_execvpe failed");
    return 1;
}
