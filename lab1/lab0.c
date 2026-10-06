#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

extern char **environ;


/* Одна найденная опция */
typedef struct
{
    char option;
    char *argument;
} Option;


/* Печать идентификаторов пользователя и группы */
void print_ids(void)
{
    printf("UID:  %d\n", (int)getuid());
    printf("EUID: %d\n", (int)geteuid());
    printf("GID:  %d\n", (int)getgid());
    printf("EGID: %d\n", (int)getegid());
}


/* Сделать процесс лидером своей группы */
void make_group_leader(void)
{
    if (setpgid(0, 0) == -1)
    {
        perror("setpgid");
    }
    else
    {
        printf("Process is now group leader\n");
    }
}


/* Печать PID, PPID и PGID */
void print_process_ids(void)
{
    printf("PID:  %d\n", (int)getpid());
    printf("PPID: %d\n", (int)getppid());
    printf("PGID: %d\n", (int)getpgrp());
}


/* Печать ulimit.
 *
 * В этой работе под ulimit будем понимать
 * ограничение размера файла RLIMIT_FSIZE.
 */
void print_ulimit(void)
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_FSIZE, &limit) == -1)
    {
        perror("getrlimit");
        return;
    }

    printf("ulimit: ");

    if (limit.rlim_cur == RLIM_INFINITY)
        printf("unlimited\n");
    else
        printf("%llu\n", (unsigned long long)limit.rlim_cur);
}


/* Изменение ulimit */
void change_ulimit(char *arg)
{
    char *end;
    long value;
    struct rlimit limit;

    errno = 0;
    end = NULL;

    value = strtol(arg, &end, 10);

    /*
     * Проверяем:
     * 1. число вообще должно быть
     * 2. после числа не должно быть мусора
     * 3. не должно быть переполнения
     * 4. значение не может быть отрицательным
     */
    if (arg[0] == '\0' ||
        *end != '\0' ||
        errno == ERANGE ||
        value < 0)
    {
        fprintf(stderr, "Invalid ulimit value: %s\n", arg);
        return;
    }

    if (getrlimit(RLIMIT_FSIZE, &limit) == -1)
    {
        perror("getrlimit");
        return;
    }

    limit.rlim_cur = (rlim_t)value;

    if (setrlimit(RLIMIT_FSIZE, &limit) == -1)
    {
        perror("setrlimit");
        return;
    }

    printf("ulimit changed to %ld\n", value);
}


/* Печать размера core-файла */
void print_core_size(void)
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_CORE, &limit) == -1)
    {
        perror("getrlimit");
        return;
    }

    printf("core size: ");

    if (limit.rlim_cur == RLIM_INFINITY)
        printf("unlimited\n");
    else
        printf("%llu bytes\n",
               (unsigned long long)limit.rlim_cur);
}


/* Изменение размера core-файла */
void change_core_size(char *arg)
{
    char *end;
    long value;
    struct rlimit limit;

    errno = 0;
    end = NULL;

    value = strtol(arg, &end, 10);

    if (arg[0] == '\0' ||
        *end != '\0' ||
        errno == ERANGE ||
        value < 0)
    {
        fprintf(stderr, "Invalid core size: %s\n", arg);
        return;
    }

    if (getrlimit(RLIMIT_CORE, &limit) == -1)
    {
        perror("getrlimit");
        return;
    }

    limit.rlim_cur = (rlim_t)value;

    if (setrlimit(RLIMIT_CORE, &limit) == -1)
    {
        perror("setrlimit");
        return;
    }

    printf("core size changed to %ld bytes\n", value);
}


/* Печать текущей директории */
void print_current_directory(void)
{
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd");
        return;
    }

    printf("Current directory: %s\n", cwd);
}


/* Печать переменных окружения */
void print_environment(void)
{
    char **env = environ;

    while (*env != NULL)
    {
        printf("%s\n", *env);
        env++;
    }
}


/* Добавление/изменение переменной окружения */
void change_environment(char *arg)
{
    char *equal;

    equal = strchr(arg, '=');

    if (equal == NULL)
    {
        fprintf(stderr,
                "Invalid environment variable: %s\n",
                arg);
        return;
    }

    /*
     * Временно заменяем '=' на '\0',
     * чтобы получить имя переменной.
     */
    *equal = '\0';

    if (arg[0] == '\0')
    {
        fprintf(stderr, "Empty environment variable name\n");
        *equal = '=';
        return;
    }

    if (setenv(arg, equal + 1, 1) == -1)
    {
        perror("setenv");
    }
    else
    {
        printf("Environment variable changed: %s=%s\n",
               arg,
               equal + 1);
    }

    *equal = '=';
}


/* Выполнение одной опции */
void execute_option(Option *opt)
{
    switch (opt->option)
    {
        case 'i':
            print_ids();
            break;

        case 's':
            make_group_leader();
            break;

        case 'p':
            print_process_ids();
            break;

        case 'u':
            print_ulimit();
            break;

        case 'U':
            change_ulimit(opt->argument);
            break;

        case 'c':
            print_core_size();
            break;

        case 'C':
            change_core_size(opt->argument);
            break;

        case 'd':
            print_current_directory();
            break;

        case 'v':
            print_environment();
            break;

        case 'V':
            change_environment(opt->argument);
            break;

        default:
            fprintf(stderr, "Unknown option: -%c\n",
                    opt->option);
    }
}


int main(int argc, char *argv[])
{
    size_t max_options = 0;
    Option *options;
    int option_count = 0;
    int c;

    /* Один аргумент может содержать несколько коротких опций: -isp. */
    for (int i = 1; i < argc; ++i)
        max_options += strlen(argv[i]);
    options = malloc((max_options ? max_options : 1) * sizeof(*options));
    if (options == NULL)
    {
        perror("malloc");
        return 1;
    }

    /*
     * :
     * если у опции нет аргумента, getopt вернет ':',
     * а не '?'.
     */
    opterr = 0;

    /*
     * i s p u U: c C: d v V:
     *
     * U, C и V имеют обязательный аргумент.
     */
    while ((c = getopt(argc, argv, ":ispuU:cC:dV:v")) != -1)
    {
        switch (c)
        {
            case 'i':
            case 's':
            case 'p':
            case 'u':
            case 'c':
            case 'd':
            case 'v':
                options[option_count].option = (char)c;
                options[option_count].argument = NULL;
                option_count++;
                break;

            case 'U':
            case 'C':
            case 'V':
                options[option_count].option = (char)c;
                options[option_count].argument = optarg;
                option_count++;
                break;

            case ':':
                fprintf(stderr,
                        "Option -%c requires an argument\n",
                        optopt);
                return 1;

            case '?':
                fprintf(stderr,
                        "Invalid option: -%c\n",
                        optopt);
                return 1;
        }
    }

    /*
     * Проверяем, не остались ли обычные аргументы.
     */
    if (optind < argc)
    {
        fprintf(stderr,
                "Unexpected argument: %s\n",
                argv[optind]);
        return 1;
    }

    /*
     * ВАЖНАЯ ЧАСТЬ ЗАДАНИЯ:
     *
     * getopt() нашел опции слева направо,
     * а выполняем мы их справа налево.
     */
    for (int i = option_count - 1; i >= 0; i--)
    {
        execute_option(&options[i]);
    }

    free(options);
    return 0;
}
