#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <ulimit.h>
#include <unistd.h>

extern char **environ;

typedef struct {
    char name;
    char *arg;
} Option;

int main(int argc, char *argv[])
{
    Option options[argc];
    int count = 0;
    int opt;

    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        if (opt == '?')
            return 1;

        options[count].name = opt;
        options[count].arg = optarg;
        count++;
    }

    for (int i = count - 1; i >= 0; i--) {
        char *arg = options[i].arg;

        switch (options[i].name) {
        case 'i':
            printf("uid=%ld euid=%ld gid=%ld egid=%ld\n",
                   (long)getuid(), (long)geteuid(),
                   (long)getgid(), (long)getegid());
            break;

        case 's':
            if (setpgid(0, 0) == -1)
                perror("setpgid");
            break;

        case 'p':
            printf("pid=%ld ppid=%ld pgid=%ld\n",
                   (long)getpid(), (long)getppid(),
                   (long)getpgrp());
            break;

        case 'u':
            printf("%ld\n", ulimit(UL_GETFSIZE));
            break;

        case 'U': {
            long value = atol(arg);

            if (ulimit(UL_SETFSIZE, value) == -1)
                perror("ulimit");
            break;
        }

        case 'c': {
            struct rlimit limit;

            if (getrlimit(RLIMIT_CORE, &limit) == -1)
                perror("getrlimit");
            else
                printf("%llu\n",
                       (unsigned long long)limit.rlim_cur);
            break;
        }

        case 'C': {
            struct rlimit limit;

            if (getrlimit(RLIMIT_CORE, &limit) == -1) {
                perror("getrlimit");
                break;
            }

            limit.rlim_cur = (rlim_t)atol(arg);

            if (setrlimit(RLIMIT_CORE, &limit) == -1)
                perror("setrlimit");

            break;
        }

        case 'd': {
            char path[PATH_MAX];

            if (getcwd(path, sizeof(path)) == NULL)
                perror("getcwd");
            else
                printf("%s\n", path);

            break;
        }

        case 'v':
            for (char **p = environ; *p != NULL; p++)
                printf("%s\n", *p);
            break;

        case 'V':
            if (putenv(arg) != 0)
                perror("putenv");
            break;
        }
    }

    return 0;
}
