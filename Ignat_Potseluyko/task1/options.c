#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

extern char **environ;

typedef struct {
    char opt;
    char *arg;
} OptEntry;

int main(int argc, char *argv[]) {
    char *options = "ispuU:cC:dvV:";
    int c;
    OptEntry *list = NULL;
    int count = 0;

    while ((c = getopt(argc, argv, options)) != -1) {
        OptEntry *tmp = realloc(list, (count + 1) * sizeof(OptEntry));
        if (!tmp) {
            perror("realloc");
            free(list);
            return 1;
        }
        list = tmp;
        list[count].opt = (char)c;
        list[count].arg = optarg ? strdup(optarg) : NULL;
        count++;
    }

    for (int i = count - 1; i >= 0; i--) {
        char opt = list[i].opt;
        char *arg = list[i].arg;

        switch (opt) {
        case 'i': {
            printf("UID: real=%d, effective=%d\n",
                   (int)getuid(), (int)geteuid());
            printf("GID: real=%d, effective=%d\n",
                   (int)getgid(), (int)getegid());
            break;
        }
        case 's': {
            if (setpgid(0, 0) == -1) {
                perror("setpgid");
            } else {
                printf("Process became group leader, PGID=%d\n",
                       (int)getpgrp());
            }
            break;
        }
        case 'p': {
            printf("PID=%d, PPID=%d, PGID=%d\n",
                   (int)getpid(), (int)getppid(), (int)getpgrp());
            break;
        }
        case 'u': {
            struct rlimit rl;
            if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
                printf("ulimit (RLIMIT_NOFILE): soft=%lu, hard=%lu\n",
                       (unsigned long)rl.rlim_cur,
                       (unsigned long)rl.rlim_max);
            } else {
                perror("getrlimit");
            }
            break;
        }
        case 'U': {
            struct rlimit rl;
            if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
                char *endptr;
                errno = 0;
                long val = strtol(arg, &endptr, 10);
                if (errno != 0 || *endptr != '\0' || val < 0) {
                    fprintf(stderr, "Invalid ulimit value: %s\n", arg);
                } else {
                    rl.rlim_cur = (rlim_t)val;
                    if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
                        perror("setrlimit");
                    } else {
                        printf("ulimit set to %ld\n", val);
                    }
                }
            }
            break;
        }
        case 'c': {
            struct rlimit rl;
            if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                printf("Core file size: soft=%lu, hard=%lu\n",
                       (unsigned long)rl.rlim_cur,
                       (unsigned long)rl.rlim_max);
            } else {
                perror("getrlimit");
            }
            break;
        }
        case 'C': {
            struct rlimit rl;
            if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                char *endptr;
                errno = 0;
                long val = strtol(arg, &endptr, 10);
                if (errno != 0 || *endptr != '\0' || val < 0) {
                    fprintf(stderr, "Invalid core size: %s\n", arg);
                } else {
                    rl.rlim_cur = (rlim_t)val;
                    if (setrlimit(RLIMIT_CORE, &rl) == -1) {
                        perror("setrlimit");
                    } else {
                        printf("Core size set to %ld\n", val);
                    }
                }
            }
            break;
        }
        case 'd': {
            char buf[PATH_MAX];
            if (getcwd(buf, sizeof(buf)) != NULL) {
                printf("Current directory: %s\n", buf);
            } else {
                perror("getcwd");
            }
            break;
        }
        case 'v': {
            printf("--- Environment variables ---\n");
            for (char **env = environ; *env; env++) {
                printf("%s\n", *env);
            }
            printf("--- End ---\n");
            break;
        }
        case 'V': {
            if (arg && strchr(arg, '=')) {
                if (putenv(arg) != 0) {
                    perror("putenv");
                } else {
                    printf("Environment variable set: %s\n", arg);
                }
            } else {
                fprintf(stderr, "Invalid -V argument: %s\n",
                        arg ? arg : "(null)");
            }
            break;
        }
        case '?':
        default:
            fprintf(stderr, "Unknown option: %c\n",
                    optopt ? optopt : '?');
            break;
        }
    }

    for (int i = 0; i < count; i++) {
        free(list[i].arg);
    }
    free(list);

    return 0;
}
