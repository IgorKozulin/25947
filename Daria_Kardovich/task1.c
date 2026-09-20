#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <ulimit.h>
#include <limits.h>

extern char **environ;

typedef struct {
    int opt;
    char *arg;
} ParsedOption;

static void handle_option(int opt, const char *arg) {
    switch (opt) {
        case 'i':
            printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
            printf("Real GID: %d, Effective GID: %d\n", getgid(), getegid());
            break;
        case 's':
            if (setpgid(0, 0) == -1) {
                perror("setpgid error");
            } else {
                printf("Process became group leader. PGID: %d\n", getpgrp());
            }
            break;
        case 'p':
            printf("PID: %d, PPID: %d, PGID: %d\n", getpid(), getppid(), getpgrp());
            break;
        case 'u':
            printf("Current ulimit (file size): %ld\n", ulimit(UL_GETFSIZE));
            break;
        case 'U': {
            long new_ulimit = atol(arg);
            if (ulimit(UL_SETFSIZE, new_ulimit) == -1) {
                perror("ulimit change error");
            } else {
                printf("New ulimit set to: %ld\n", new_ulimit);
            }
            break;
        }
        case 'c': {
            struct rlimit rl;
            if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                printf("Core file size: %ld bytes\n", (long)rl.rlim_cur);
            } else {
                perror("getrlimit error");
            }
            break;
        }
        case 'C': {
            long new_core = atol(arg);
            struct rlimit rl;
            if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                rl.rlim_cur = new_core;
                if (setrlimit(RLIMIT_CORE, &rl) == 0) {
                    printf("New core file size set to: %ld\n", new_core);
                } else {
                    perror("setrlimit error");
                }
            }
            break;
        }
        case 'd': {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("Current directory: %s\n", cwd);
            } else {
                perror("getcwd error");
            }
            break;
        }
        case 'v': {
   
            char **env;
            for (env = environ; *env != NULL; ++env) {
                printf("%s\n", *env);
            }
            break;
        }
        case 'V': {
            if (strchr(arg, '=') == NULL) {
                fprintf(stderr, "Error: -V argument must be NAME=VALUE\n");
                break;
            }
            char *env_copy = strdup(arg); 
            if (putenv(env_copy) != 0) {
                perror("putenv error");
                free(env_copy);
            } else {
                printf("Environment variable updated: %s\n", arg);
            }
            break;
        }
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Usage: %s [-i] [-s] [-p] [-u] [-U value] [-c] [-C size] [-d] [-v] [-V name=value]\n", argv[0]);
        return 0;
    }

    int c;
    ParsedOption *opts = malloc((size_t)argc * sizeof(ParsedOption));
    int opt_count = 0;
    int i; 

    while ((c = getopt(argc, argv, "ispudvU:cC:V:")) != -1) {
        if (c == '?') continue;
        opts[opt_count].opt = c;
        opts[opt_count].arg = optarg ? optarg : NULL;
        opt_count++;
    }
    for (i = opt_count - 1; i >= 0; --i) {
        handle_option(opts[i].opt, opts[i].arg);
    }

    free(opts);
    return 0;
}
