#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <errno.h>
#include <string.h>

extern char **environ;

int parse_long(const char *str, long *result) {
    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {
        return -1;
    }
    if (endptr == str) {
        return -1; // Не найдено ни одной цифры
    }
    if (*endptr != '\0') {
        return -1; 
    }

    *result = val;
    return 0;
}

int main(int argc, char *argv[]) {
    for (int i = argc - 1; i > 0; i--) {
        char *arg = argv[i];

        if (arg[0] == '-') {
            switch (arg[1]) {
                case 'i': {
                    printf("UID: real=%d, effective=%d\n", getuid(), geteuid());
                    printf("GID: real=%d, effective=%d\n", getgid(), getegid());
                    break;
                }
                case 's': {
                    if (setsid() < 0) {
                        perror("setsid error");
                    } else {
                        printf("Process became session leader, SID=%d\n", getsid(0));
                    }
                    break;
                }
                case 'p': {
                    printf("PID=%d, PPID=%d, PGID=%d\n", getpid(), getppid(), getpgrp());
                    break;
                }
                case 'u': {
                    struct rlimit rl;
                    if (getrlimit(RLIMIT_FSIZE, &rl) == 0) {
                        printf("Current ulimit (FSIZE): %lu\n", (unsigned long)rl.rlim_cur);
                    } else {
                        perror("getrlimit error");
                    }
                    break;
                }
                case 'c': {
                    struct rlimit rl;
                    if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                        printf("Current core limit: %lu\n", (unsigned long)rl.rlim_cur);
                    } else {
                        perror("getrlimit error");
                    }
                    break;
                }
                case 'd': {
                    char cwd[1024];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        printf("Current working directory: %s\n", cwd);
                    } else {
                        perror("getcwd error");
                    }
                    break;
                }
                case 'v': {
                    printf("Environment variables:\n");
                    for (char **env = environ; *env != 0; env++) {
                        printf("  %s\n", *env);
                    }
                    break;
                }
                case 'U':
                case 'C':
                case 'V':
                case 'i': // если бы были другие
                    break;
                default:
                    fprintf(stderr, "Unknown option: %s\n", arg);
                    break;
            }
        }

        if (arg[0] == '-' && (arg[1] == 'U' || arg[1] == 'C' || arg[1] == 'V')) {
            if (i + 1 < argc) {
                char *val_arg = argv[i + 1];
                if (arg[1] == 'U') {
                    long val;
                    if (parse_long(val_arg, &val) == 0) {
                        struct rlimit rl;
                        rl.rlim_cur = (rlim_t)val;
                        rl.rlim_max = (rlim_t)val;
                        if (setrlimit(RLIMIT_FSIZE, &rl) < 0) {
                            perror("setrlimit FSIZE error");
                        } else {
                            printf("Successfully set ulimit (FSIZE) to %ld\n", val);
                        }
                    } else {
                        fprintf(stderr, "Invalid numeric argument for -U: %s\n", val_arg);
                    }
                } else if (arg[1] == 'C') {
                    long val;
                    if (parse_long(val_arg, &val) == 0) {
                        struct rlimit rl;
                        rl.rlim_cur = (rlim_t)val;
                        rl.rlim_max = (rlim_t)val;
                        if (setrlimit(RLIMIT_CORE, &rl) < 0) {
                            perror("setrlimit CORE error");
                        } else {
                            printf("Successfully set core limit to %ld\n", val);
                        }
                    } else {
                        fprintf(stderr, "Invalid numeric argument for -C: %s\n", val_arg);
                    }
                } else if (arg[1] == 'V') {
                    if (putenv(val_arg) != 0) {
                        perror("putenv error");
                    } else {
                        printf("Successfully set environment variable: %s\n", val_arg);
                    }
                }
                i--;
            } else {
                fprintf(stderr, "Option -%c requires an argument\n", arg[1]);
            }
        }
    }

    return 0;
}
