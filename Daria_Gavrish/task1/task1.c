#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

extern char **environ;

void print_ids(void)
{
    printf("Real UID: %d\n", (int)getuid());
    printf("Effective UID: %d\n", (int)geteuid());
    printf("Real GID: %d\n", (int)getgid());
    printf("Effective GID: %d\n", (int)getegid());
}

void print_process_info(void)
{
    printf("PID: %d\n", (int)getpid());
    printf("PPID: %d\n", (int)getppid());
    printf("PGID: %d\n", (int)getpgrp());
}

void print_ulimit(void)
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_FSIZE, &limit) == -1)
    {
        perror("getrlimit");
        return;
    }

    if (limit.rlim_cur == RLIM_INFINITY)
        printf("Ulimit: unlimited\n");
    else
        printf("Ulimit: %ld\n", (long)limit.rlim_cur);
}

void set_ulimit_value(const char *value)
{
    long number;
    char *endptr;
    struct rlimit limit;

    errno = 0;
    endptr = NULL;
    number = strtol(value, &endptr, 10);

    if (errno != 0 || endptr == value || *endptr != '\0' || number < 0)
    {
        printf("Invalid value for -U: %s\n", value);
        return;
    }

    limit.rlim_cur = (rlim_t)number;
    limit.rlim_max = (rlim_t)number;

    if (setrlimit(RLIMIT_FSIZE, &limit) == -1)
        perror("setrlimit");
    else
        printf("New ulimit: %ld\n", number);
}

void print_core_size(void)
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_CORE, &limit) == -1)
    {
        perror("getrlimit");
        return;
    }

    if (limit.rlim_cur == RLIM_INFINITY)
        printf("Core file size: unlimited\n");
    else
        printf("Core file size: %ld bytes\n", (long)limit.rlim_cur);
}

void set_core_size(const char *value)
{
    long number;
    char *endptr;
    struct rlimit limit;

    errno = 0;
    endptr = NULL;
    number = strtol(value, &endptr, 10);

    if (errno != 0 || endptr == value || *endptr != '\0' || number < 0)
    {
        printf("Invalid value for -C: %s\n", value);
        return;
    }

    limit.rlim_cur = (rlim_t)number;
    limit.rlim_max = (rlim_t)number;

    if (setrlimit(RLIMIT_CORE, &limit) == -1)
        perror("setrlimit");
    else
        printf("New core file size: %ld bytes\n", number);
}

void print_directory(void)
{
    char cwd[4096];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        perror("getcwd");
    else
        printf("Current directory: %s\n", cwd);
}

void print_environment(void)
{
    char **env;

    env = environ;

    while (*env != NULL)
    {
        printf("%s\n", *env);
        env++;
    }
}

void set_environment_value(const char *value)
{
    char *copy;

    copy = malloc(strlen(value) + 1);

    if (copy == NULL)
    {
        perror("malloc");
        return;
    }

    strcpy(copy, value);

    if (putenv(copy) != 0)
        perror("putenv");
    else
        printf("Environment variable changed: %s\n", value);
}

void print_california_time(void)
{
    time_t now;
    struct tm *time_info;

    putenv("TZ=PST8");
    tzset();

    time(&now);

    printf("Californian time:\n");
    printf("%s", ctime(&now));

    time_info = localtime(&now);

    printf("Formatted: %d/%d/%02d %d:%02d:%02d %s\n",
           time_info->tm_mon + 1,
           time_info->tm_mday,
           time_info->tm_year,
           time_info->tm_hour,
           time_info->tm_min,
           time_info->tm_sec,
           tzname[time_info->tm_isdst]);
}

int main(int argc, char *argv[])
{
    char **reversed_argv;
    int i;
    int opt;

    if (argc <= 1)
        return 0;

    reversed_argv = malloc(sizeof(char *) * argc);

    if (reversed_argv == NULL)
    {
        perror("malloc");
        return 1;
    }

    reversed_argv[0] = argv[0];

    i = 1;
    while (i < argc)
    {
        reversed_argv[i] = argv[argc - i];
        i++;
    }

    optind = 1;

    while ((opt = getopt(argc, reversed_argv, "+isputU:cC:dvV:")) != -1)
    {
        switch (opt)
        {
            case 'i':
                print_ids();
                break;

            case 's':
                if (setpgid(0, 0) == -1)
                    perror("setpgid");
                else
                    printf("Process became group leader.\n");
                break;

            case 'p':
                print_process_info();
                break;

            case 'u':
                print_ulimit();
                break;

            case 't':
                print_california_time();
                break;

            case 'U':
                set_ulimit_value(optarg);
                break;

            case 'c':
                print_core_size();
                break;

            case 'C':
                set_core_size(optarg);
                break;

            case 'd':
                print_directory();
                break;

            case 'v':
                print_environment();
                break;

            case 'V':
                set_environment_value(optarg);
                break;

            case '?':
                printf("Unknown or invalid option\n");
                break;

            default:
                break;
        }
    }

    free(reversed_argv);

    return 0;
}
