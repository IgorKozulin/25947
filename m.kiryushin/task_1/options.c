#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>

extern char **environ;

int main(int argc, char **argv)
{
    char *options = "isp uU:cC:dvV:";
    int opt;

    while ((opt = getopt(argc, argv, options)) != -1)
    {
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

            case '?':
            {
                printf("invalid option\n");
                break;
            }
        }
    }

    return 0;
}
