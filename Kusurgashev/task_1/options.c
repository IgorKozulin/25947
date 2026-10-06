#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <limits.h>
#include <errno.h>

extern char **environ;

int parse_value(const char *str, long *value)
{
    char *end;

    errno = 0;
    *value = strtol(str, &end, 10);

    if (errno != 0 || end == str || *end != '\0' || *value < 0)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    int opt;
    int options[128];
    char *args[128];
    int count = 0;

    opterr = 0;

    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1)
    {
        if (opt == '?')
        {
            if (optopt)
                fprintf(stderr, "Invalid option: -%c\n", optopt);
            else
                fprintf(stderr, "Invalid option\n");

            return 1;
        }

        options[count] = opt;
        args[count] = optarg;
        count++;
    }

    for (int i = count - 1; i >= 0; i--)
    {
        switch (options[i])
        {
            case 'i':
                printf("Real UID: %d\n", getuid());
                printf("Effective UID: %d\n", geteuid());
                printf("Real GID: %d\n", getgid());
                printf("Effective GID: %d\n", getegid());
                break;

            case 's':
                if (setpgid(0, 0) == -1)
                    perror("setpgid");
                break;

            case 'p':
                printf("PID: %d\n", getpid());
                printf("PPID: %d\n", getppid());
                printf("PGID: %d\n", getpgrp());
                break;

            case 'u':
            {
                struct rlimit limit;

                if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
                {
                    perror("getrlimit");
                    break;
                }

                if (limit.rlim_cur == RLIM_INFINITY)
                    printf("ulimit: unlimited\n");
                else
                    printf("ulimit: %lu\n",
                           (unsigned long)limit.rlim_cur);

                break;
            }

            case 'U':
            {
                long value;
                struct rlimit limit;

                if (parse_value(args[i], &value) == -1)
                {
                    fprintf(stderr, "Invalid value for -U: %s\n", args[i]);
                    break;
                }

                if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
                {
                    perror("getrlimit");
                    break;
                }

                if ((unsigned long)value > (unsigned long)limit.rlim_max)
                {
                    fprintf(stderr, "Value for -U is greater than hard limit\n");
                    break;
                }

                limit.rlim_cur = (rlim_t)value;

                if (setrlimit(RLIMIT_NOFILE, &limit) == -1)
                    perror("setrlimit");

                break;
            }

            case 'c':
            {
                struct rlimit limit;

                if (getrlimit(RLIMIT_CORE, &limit) == -1)
                {
                    perror("getrlimit");
                    break;
                }

                if (limit.rlim_cur == RLIM_INFINITY)
                    printf("core: unlimited\n");
                else
                    printf("core: %lu bytes\n",
                           (unsigned long)limit.rlim_cur);

                break;
            }

            case 'C':
            {
                long value;
                struct rlimit limit;

                if (parse_value(args[i], &value) == -1)
                {
                    fprintf(stderr, "Invalid value for -C: %s\n", args[i]);
                    break;
                }

                if (getrlimit(RLIMIT_CORE, &limit) == -1)
                {
                    perror("getrlimit");
                    break;
                }

                if ((unsigned long)value > (unsigned long)limit.rlim_max)
                {
                    fprintf(stderr, "Value for -C is greater than hard limit\n");
                    break;
                }

                limit.rlim_cur = (rlim_t)value;

                if (setrlimit(RLIMIT_CORE, &limit) == -1)
                    perror("setrlimit");

                break;
            }

            case 'd':
            {
                char cwd[PATH_MAX];

                if (getcwd(cwd, sizeof(cwd)) == NULL)
                    perror("getcwd");
                else
                    printf("%s\n", cwd);

                break;
            }

            case 'v':
            {
                char **env = environ;

                while (*env != NULL)
                {
                    printf("%s\n", *env);
                    env++;
                }

                break;
            }

            case 'V':
                if (putenv(args[i]) == -1)
                    perror("putenv");
                break;
        }
    }

    return 0;
}
