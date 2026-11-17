#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>

extern char **environ;

int main(int argc, char **argv)
{
    char *options = "ispuU:cC:dtvV:";
    int opt;
    struct option_entry
    {
        int value;
        char *argument;
    };
    struct option_entry *parsed_options = calloc((size_t)argc, sizeof(*parsed_options));
    int option_count = 0;

    if (parsed_options == NULL)
    {
        perror("calloc");
        return 1;
    }

    while ((opt = getopt(argc, argv, options)) != -1)
    {
        parsed_options[option_count].value = opt;
        parsed_options[option_count].argument = optarg;
        option_count++;
    }

    for (int i = option_count - 1; i >= 0; i--)
    {
        opt = parsed_options[i].value;
        optarg = parsed_options[i].argument;
        switch (opt)
        {
            case 'i':
            {
                printf("UID: %d\n", getuid());
                printf("GID: %d\n", getgid());
                break;
            }
            case 's':
            {
                if (setpgid(0, 0) == -1)
                {
                    perror("setpgid");
                    break;
                }

                printf("process group leader\n");
                break;
            }
            case 'p':
            {
                printf("PID: %d\n", getpid());
                printf("PPID: %d\n", getppid());
                printf("PGID: %d\n", getpgrp());
                break;
            }
            case 'u':
            {
                struct rlimit limit;

                if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
                {
                    perror("getrlimit");
                    break;
                }

                printf("ulimit: %ld\n", limit.rlim_cur);
                break;
            }
            case 'U':
            {
                long value = atol(optarg);
                struct rlimit limit;

                if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
                {
                    perror("getrlimit");
                    break;
                }

                limit.rlim_cur = value;

                if (setrlimit(RLIMIT_NOFILE, &limit) == -1)
                {
                    perror("setrlimit");
                    break;
                }

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

                printf("core: %ld\n", limit.rlim_cur);
                break;
            }
            case 'C':
            {
                long value = atol(optarg);
                struct rlimit limit;

                if (getrlimit(RLIMIT_CORE, &limit) == -1)
                {
                    perror("getrlimit");
                    break;
                }

                limit.rlim_cur = value;

                if (setrlimit(RLIMIT_CORE, &limit) == -1)
                {
                    perror("setrlimit");
                    break;
                }

                break;
            }
            case 'd':
            {
                char cwd[1024];

                if (getcwd(cwd, sizeof(cwd)) == NULL)
                {
                    perror("getcwd");
                    break;
                }

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
            {
                if (putenv(optarg) == -1)
                {
                    perror("putenv");
                    break;
                }

                break;
            }
            case 't':
            {
                setenv("TZ", "America/Los_Angeles", 1);
                tzset();
                time_t now;
                time(&now);
                struct tm *sp = localtime(&now);
                printf("%02d/%02d/%04d %02d:%02d:%02d %s\n",
                    sp->tm_mday,
                    sp->tm_mon + 1,
                    sp->tm_year + 1900,
                    sp->tm_hour,
                    sp->tm_min,
                    sp->tm_sec,
                    tzname[sp->tm_isdst]);
                break;
            }

            case '?':
            {
                printf("invalid option\n");
                break;
            }
        }
    }

    free(parsed_options);
    return 0;
}
