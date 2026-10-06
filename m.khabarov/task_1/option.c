#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

extern char **environ;

typedef struct {
    char option;
    char *argument;
} ParsedOption;

int main(int argc, char *argv[]) {
    const char *opt_string = "ispuU:cC:dvV:";
    int c;
    int count = 0;
    int capacity = 10;
    int i, k; // Объявляем переменные для циклов заранее
    char **env; // Объявляем указатель для цикла по окружению

    ParsedOption *options_list = malloc(capacity * sizeof(ParsedOption));
    if (!options_list) {
        perror("malloc");
        return 1;
    }

    while ((c = getopt(argc, argv, opt_string)) != -1) {
        if (count >= capacity) {
            capacity *= 2;
            ParsedOption *temp = realloc(options_list, capacity * sizeof(ParsedOption));
            if (!temp) {
                perror("realloc");
                free(options_list);
                return 1;
            }
            options_list = temp;
        }

        options_list[count].option = c;
        options_list[count].argument = NULL;

        if (optarg) {
            options_list[count].argument = strdup(optarg);
            if (!options_list[count].argument) {
                perror("strdup");
                for (k = 0; k < count; k++) free(options_list[k].argument);
                free(options_list);
                return 1;
            }
        }
        count++;
    }

    for (i = count - 1; i >= 0; i--) {
        char opt = options_list[i].option;
        char *arg = options_list[i].argument;

        switch (opt) {
            case 'i': {
                printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
                printf("Real GID: %d, Effective GID: %d\n", getgid(), getegid());
                break;
            }
            case 's': {
                if (setpgid(0, 0) == -1) {
                    perror("setpgid failed");
                } else {
                    printf("Process became group leader. New PGID: %d\n", getpgid(0));
                }
                break;
            }
            case 'p': {
                printf("PID: %d, PPID: %d, PGID: %d\n", getpid(), getppid(), getpgid(0));
                break;
            }
            case 'u': {
                struct rlimit rl;
                if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
                    printf("Current ulimit (RLIMIT_NOFILE): soft=%ld, hard=%ld\n",
                           (long)rl.rlim_cur, (long)rl.rlim_max);
                } else {
                    perror("getrlimit failed");
                }
                break;
            }
            case 'U': {
                if (!arg) {
                    fprintf(stderr, "Error: Option -U requires an argument.\n");
                    break;
                }
                long new_limit = strtol(arg, NULL, 10);
                if (new_limit < 0) {
                    fprintf(stderr, "Error: Invalid limit value '%s'. Must be non-negative.\n", arg);
                } else {
                    struct rlimit rl;
                    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
                        rl.rlim_cur = (rlim_t)new_limit;
                        if (rl.rlim_cur > rl.rlim_max) {
                            rl.rlim_max = rl.rlim_cur;
                        }
                        if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
                            perror("setrlimit failed");
                        } else {
                            printf("Ulimit changed to %ld\n", new_limit);
                        }
                    } else {
                        perror("getrlimit failed before setrlimit");
                    }
                }
                break;
            }
            case 'c': {
                struct rlimit rl;
                if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                    printf("Core file size limit: soft=%ld, hard=%ld bytes\n",
                           (long)rl.rlim_cur, (long)rl.rlim_max);
                } else {
                    perror("getrlimit(RLIMIT_CORE) failed");
                }
                break;
            }
            case 'C': {
                if (!arg) {
                    fprintf(stderr, "Error: Option -C requires an argument.\n");
                    break;
                }
                long new_size = strtol(arg, NULL, 10);
                if (new_size < 0) {
                    fprintf(stderr, "Error: Invalid core size '%s'. Must be non-negative.\n", arg);
                } else {
                    struct rlimit rl;
                    rl.rlim_cur = (rlim_t)new_size;
                    rl.rlim_max = (rlim_t)new_size;
                    if (setrlimit(RLIMIT_CORE, &rl) == -1) {
                        perror("setrlimit(RLIMIT_CORE) failed");
                    } else {
                        printf("Core file size limit changed to %ld bytes\n", new_size);
                    }
                }
                break;
            }
            case 'd': {
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("Current working directory: %s\n", cwd);
                } else {
                    perror("getcwd failed");
                }
                break;
            }
            case 'v': {
                printf("--- Environment Variables ---\n");
                for (env = environ; *env != NULL; env++) {
                    printf("%s\n", *env);
                }
                printf("---------------------------\n");
                break;
            }
            case 'V': {
                if (!arg) {
                    fprintf(stderr, "Error: Option -V requires an argument in format NAME=VALUE.\n");
                    break;
                }
                if (strchr(arg, '=') == NULL) {
                    fprintf(stderr, "Error: Invalid format for -V. Use NAME=VALUE.\n");
                } else {
                    if (putenv(arg) != 0) {
                        perror("putenv failed");
                    } else {
                        printf("Environment variable set: %s\n", arg);
                    }
                }
                break;
            }
            case '?': {
                fprintf(stderr, "Invalid option detected during parsing.\n");
                break;
            }
            default:
                break;
        }
    }

    for (i = 0; i < count; i++) {
        free(options_list[i].argument);
    }
    free(options_list);

    return 0;
}