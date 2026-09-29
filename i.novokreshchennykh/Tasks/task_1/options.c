#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

extern char **environ;

struct option_item {
    int option;
    char *arg;
};

int main(int argc, char *argv[])
{
    struct rlimit limit;

    char *options = "inspuU:cC:dvV:";
    int chr;

    struct option_item *items = NULL;
    size_t count = 0;

    while ((chr = getopt(argc, argv, options)) != -1)
    {
        if (chr == '?')
        {
            printf("Invalid option: %c\n", optopt);
            free(items);
            return 1;
        }

        struct option_item *tmp = realloc(items, (count + 1) * sizeof(*items));

        if (tmp == NULL)
        {
            perror("realloc");
            free(items);
            return 1;
        }

        items = tmp;
        items[count].option = chr;
        items[count].arg = optarg ? strdup(optarg) : NULL;

        if (optarg != NULL && items[count].arg == NULL)
        {
            perror("strdup");
            for (size_t i = 0; i < count; i++)
                free(items[i].arg);
            free(items);
            return 1;
        }

        count++;
    }

    while (count > 0)
    {
        struct option_item item = items[--count];

        switch (item.option)
        {
            case 'i':
                printf("uid %d; euid %d; gid %d; egid %d\n",
                       getuid(), geteuid(), getgid(), getegid());
                break;

            case 's':
                if (setpgid(0, 0) == -1)
                    perror("setpgid");
                break;

            case 'p':
                printf("pid %d; ppid %d; pgid %d\n",
                       getpid(), getppid(), getpgrp());
                break;

            case 'u':
                if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
                    perror("getrlimit");
                else
                    printf("ulimit %ld\n", (long)limit.rlim_cur);
                break;

            case 'U':
            {
                char *end;
                errno = 0;
                long value = strtol(item.arg, &end, 10);

                if (errno != 0 || end == item.arg || *end != '\0' ||
                    value < 0)
                {
                    fprintf(stderr, "Invalid value for -U: %s\n",
                            item.arg);
                    break;
                }

                if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
                {
                    perror("getrlimit");
                    break;
                }

                limit.rlim_cur = (rlim_t)value;
                if (setrlimit(RLIMIT_NOFILE, &limit) == -1)
                    perror("setrlimit");
                break;
            }

            case 'c':
                if (getrlimit(RLIMIT_CORE, &limit) == -1)
                    perror("getrlimit");
                else
                    printf("core size %ld\n", (long)limit.rlim_cur);
                break;

            case 'C':
            {
                char *end;
                errno = 0;
                long value = strtol(item.arg, &end, 10);

                if (errno != 0 || end == item.arg || *end != '\0' ||
                    value < 0)
                {
                    fprintf(stderr, "Invalid value for -C: %s\n",
                            item.arg);
                    break;
                }

                if (getrlimit(RLIMIT_CORE, &limit) == -1)
                {
                    perror("getrlimit");
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
                for (char **env = environ; *env != NULL; env++)
                    printf("%s\n", *env);
                break;

            case 'V':
            {
                char *equals = strchr(item.arg, '=');

                if (equals == NULL || equals == item.arg)
                {
                    fprintf(stderr,
                            "Invalid value for -V; expected name=value: %s\n",
                            item.arg);
                    break;
                }

                size_t name_length = (size_t)(equals - item.arg);
                char *name = malloc(name_length + 1);

                if (name == NULL)
                {
                    perror("malloc");
                    break;
                }

                memcpy(name, item.arg, name_length);
                name[name_length] = '\0';

                if (setenv(name, equals + 1, 1) == -1)
                    perror("setenv");

                free(name);
                break;
            }
        }

        free(item.arg);
    }

    free(items);
    return 0;
}  
