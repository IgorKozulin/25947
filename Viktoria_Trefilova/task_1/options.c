#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/resource.h>
#include <ulimit.h>

extern char **environ;

int main(int argc, char **argv) {
    char opt;
    int opts[128], n = 0;
    char *args[128];

    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        if (opt == '?') {
            fprintf(stderr, "Unknown option: -%c\n", optopt);
            return 1;
        }
        opts[n] = opt;
        args[n] = optarg;
        n++;
    }

    for (int i = n - 1; i >= 0; i--) {
        switch (opts[i]) {
        case 'i':
            printf("uid=%d euid=%d gid=%d egid=%d\n",
                   getuid(), geteuid(), getgid(), getegid());
            break;
        case 's':
            setpgid(0, 0);
            break;
        case 'p':
            printf("pid=%d ppid=%d pgid=%d\n",
                   getpid(), getppid(), getpgrp());
            break;
        case 'u':
            printf("ulimit=%ld\n", ulimit(UL_GETFSIZE));
            break;
        case 'U':
            ulimit(UL_SETFSIZE, atol(args[i]));
            break;
        case 'c': {
            struct rlimit r;
            getrlimit(RLIMIT_CORE, &r);
            printf("core=%lu\n", (unsigned long)r.rlim_cur);
            break;
        }
        case 'C': {
            struct rlimit r;
            getrlimit(RLIMIT_CORE, &r);
            r.rlim_cur = atol(args[i]);
            setrlimit(RLIMIT_CORE, &r);
            break;
        }
        case 'd': {
            char cwd[PATH_MAX];
            getcwd(cwd, sizeof cwd);
            puts(cwd);
            break;
        }
        case 'v':
            for (char **p = environ; *p; p++)
                puts(*p);
            break;
        case 'V': {
            char *eq = strchr(args[i], '=');
            if (eq) {
                *eq = '\0';
                setenv(args[i], eq + 1, 1);
            }
            break;
        }
        }
    }
    return 0;
}
