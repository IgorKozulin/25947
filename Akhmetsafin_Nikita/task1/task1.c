#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <limits.h>
#include <errno.h>

extern char **environ;

int main(int argc, char *argv[]) {
    int opt;
    long val;
    char *endptr;
    struct rlimit rl;
    char cwd[PATH_MAX];

    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        switch (opt) {
            case 'i':
                printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
                printf("Real GID: %d, Effective GID: %d\n", getgid(), getegid());
                break;
            case 's':
                if (setpgid(0, 0) == -1) perror("setpgid error");
                else printf("Process is now the group leader.\n");
                break;
            case 'p':
                printf("PID: %d, PPID: %d, PGID: %d\n", getpid(), getppid(), getpgrp());
                break;
            case 'u':
                if (getrlimit(RLIMIT_FSIZE, &rl) == 0) {
                    if (rl.rlim_cur == RLIM_INFINITY) printf("ulimit (file size limit): unlimited\n");
                    else printf("ulimit (file size limit): %lu bytes\n", rl.rlim_cur);
                } else perror("getrlimit ulimit");
                break;
            case 'U':
                val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || val < 0) {
                    fprintf(stderr, "Error: Invalid value for -U ('%s')\n", optarg);
                } else if (getrlimit(RLIMIT_FSIZE, &rl) == 0) {
                    rl.rlim_cur = val;
                    if (setrlimit(RLIMIT_FSIZE, &rl) == -1) perror("setrlimit ulimit");
                    else printf("ulimit changed to %ld bytes.\n", val);
                }
                break;
            case 'c':
                if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                    if (rl.rlim_cur == RLIM_INFINITY) printf("Core file size limit: unlimited\n");
                    else printf("Core file size limit: %lu bytes\n", rl.rlim_cur);
                } else perror("getrlimit core");
                break;
            case 'C':
                val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || val < 0) {
                    fprintf(stderr, "Error: Invalid value for -C ('%s')\n", optarg);
                } else if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                    rl.rlim_cur = val;
                    if (setrlimit(RLIMIT_CORE, &rl) == -1) perror("setrlimit core");
                    else printf("Core limit changed to %ld bytes.\n", val);
                }
                break;
            case 'd':
                if (getcwd(cwd, sizeof(cwd)) != NULL) printf("Current working directory: %s\n", cwd);
                else perror("getcwd error");
                break;
            case 'v':
                printf("--- Environment Variables ---\n");
                for (char **env = environ; *env != 0; env++) printf("%s\n", *env);
                printf("-----------------------------\n");
                break;
            case 'V':
                if (putenv(optarg) != 0) perror("putenv error");
                else printf("Environment variable updated: %s\n", optarg);
                break;
            case '?':
                break;
            default:
                fprintf(stderr, "Usage: %s [-i] [-s] [-p] [-u] [-U new_ulimit] [-c] [-C size] [-d] [-v] [-V name=value]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    return 0;
}
