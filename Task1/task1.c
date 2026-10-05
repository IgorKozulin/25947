#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <ulimit.h>
#include <unistd.h>

extern char **environ;

struct operation {
    int option;
    char *argument;
};

static void
usage(const char *program)
{
    fprintf(stderr,
        "Usage: %s [-i] [-s] [-p] [-u] [-U limit] [-c] [-C size] "
        "[-d] [-v] [-V name=value] ...\n", program);
}

static int
remember_operation(struct operation **operations, size_t *count,
    int option, const char *argument)
{
    struct operation *new_operations;
    char *argument_copy = NULL;

    if (argument != NULL) {
        size_t length = strlen(argument) + 1;
        argument_copy = malloc(length);
        if (argument_copy == NULL) {
            perror("malloc");
            return -1;
        }
        memcpy(argument_copy, argument, length);
    }

    new_operations = realloc(*operations,
        (*count + 1) * sizeof(**operations));
    if (new_operations == NULL) {
        free(argument_copy);
        perror("realloc");
        return -1;
    }

    *operations = new_operations;
    (*operations)[*count].option = option;
    (*operations)[*count].argument = argument_copy;
    ++*count;
    return 0;
}

static int
parse_nonnegative_long(const char *text, long *value)
{
    char *end;
    long parsed;

    if (text == NULL || *text == '\0' || *text == '-') {
        return -1;
    }

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || *end != '\0' || parsed < 0) {
        return -1;
    }

    *value = parsed;
    return 0;
}

static int
parse_core_limit(const char *text, rlim_t *value)
{
    char *end;
    unsigned long long parsed;
    rlim_t converted;

    if (text == NULL || *text == '\0' || *text == '-') {
        return -1;
    }
    if (strcmp(text, "unlimited") == 0) {
        *value = RLIM_INFINITY;
        return 0;
    }

    errno = 0;
    parsed = strtoull(text, &end, 10);
    if (errno != 0 || *end != '\0') {
        return -1;
    }

    converted = (rlim_t)parsed;
    if ((unsigned long long)converted != parsed) {
        return -1;
    }
    *value = converted;
    return 0;
}

static int
print_core_limit(void)
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_CORE, &limit) == -1) {
        perror("getrlimit(RLIMIT_CORE)");
        return -1;
    }
    if (limit.rlim_cur == RLIM_INFINITY) {
        puts("core file limit: unlimited");
    } else {
        printf("core file limit: %llu bytes\n",
            (unsigned long long)limit.rlim_cur);
    }
    return 0;
}

static int
set_core_limit(const char *text)
{
    struct rlimit limit;
    rlim_t requested;

    if (parse_core_limit(text, &requested) == -1) {
        fprintf(stderr, "Invalid core file size: %s\n", text);
        return -1;
    }
    if (getrlimit(RLIMIT_CORE, &limit) == -1) {
        perror("getrlimit(RLIMIT_CORE)");
        return -1;
    }

    limit.rlim_cur = requested;
    if (setrlimit(RLIMIT_CORE, &limit) == -1) {
        perror("setrlimit(RLIMIT_CORE)");
        return -1;
    }
    return print_core_limit();
}

static int
set_file_limit(const char *text)
{
    long requested;
    long result;

    if (parse_nonnegative_long(text, &requested) == -1) {
        fprintf(stderr, "Invalid ulimit value: %s\n", text);
        return -1;
    }

    errno = 0;
    result = ulimit(UL_SETFSIZE, requested);
    if (result == -1) {
        perror("ulimit(UL_SETFSIZE)");
        return -1;
    }
    printf("file size limit: %ld (512-byte blocks)\n", result);
    return 0;
}

static int
set_environment_variable(const char *assignment)
{
    const char *equals = strchr(assignment, '=');
    size_t name_length;
    char *name;
    int status = 0;

    if (equals == NULL || equals == assignment) {
        fprintf(stderr, "Expected NAME=VALUE after -V, got: %s\n",
            assignment);
        return -1;
    }

    name_length = (size_t)(equals - assignment);
    name = malloc(name_length + 1);
    if (name == NULL) {
        perror("malloc");
        return -1;
    }
    memcpy(name, assignment, name_length);
    name[name_length] = '\0';

    if (setenv(name, equals + 1, 1) == -1) {
        perror("setenv");
        status = -1;
    } else {
        printf("environment: %s=%s\n", name, equals + 1);
    }
    free(name);
    return status;
}

static int
process_operation(const struct operation *operation)
{
    char directory[PATH_MAX];
    long limit;

    switch (operation->option) {
    case 'i':
        printf("uid: real=%lu effective=%lu; gid: real=%lu effective=%lu\n",
            (unsigned long)getuid(), (unsigned long)geteuid(),
            (unsigned long)getgid(), (unsigned long)getegid());
        return 0;
    case 's':
        if (getpgrp() != getpid() && setpgid(0, 0) == -1) {
            perror("setpgid");
            return -1;
        } else {
            printf("process %ld is a process-group leader\n",
                (long)getpid());
        }
        return 0;
    case 'p':
        printf("pid=%ld ppid=%ld pgid=%ld\n", (long)getpid(),
            (long)getppid(), (long)getpgrp());
        return 0;
    case 'u':
        errno = 0;
        limit = ulimit(UL_GETFSIZE);
        if (limit == -1) {
            perror("ulimit(UL_GETFSIZE)");
            return -1;
        } else {
            printf("file size limit: %ld (512-byte blocks)\n", limit);
        }
        return 0;
    case 'U':
        return set_file_limit(operation->argument);
    case 'c':
        return print_core_limit();
    case 'C':
        return set_core_limit(operation->argument);
    case 'd':
        if (getcwd(directory, sizeof(directory)) == NULL) {
            perror("getcwd");
            return -1;
        } else {
            printf("working directory: %s\n", directory);
        }
        return 0;
    case 'v':
        for (char **variable = environ; *variable != NULL; ++variable) {
            puts(*variable);
        }
        return 0;
    case 'V':
        return set_environment_variable(operation->argument);
    }
    return -1;
}

int
main(int argc, char **argv)
{
    struct operation *operations = NULL;
    size_t operation_count = 0;
    int option;
    int status = EXIT_SUCCESS;

    if (argc == 1) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }

    opterr = 0;
    while ((option = getopt(argc, argv, ":ispuU:cC:dvV:")) != -1) {
        if (option == ':' || option == '?') {
            if (option == ':') {
                fprintf(stderr, "Option -%c requires an argument\n",
                    optopt);
            } else {
                fprintf(stderr, "Invalid option: -%c\n", optopt);
            }
            usage(argv[0]);
            status = EXIT_FAILURE;
            goto cleanup;
        }
        {
            const char *argument =
                (option == 'U' || option == 'C' || option == 'V')
                    ? optarg : NULL;
            if (remember_operation(&operations, &operation_count,
                    option, argument) == -1) {
                status = EXIT_FAILURE;
                goto cleanup;
            }
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Unexpected operand: %s\n", argv[optind]);
        usage(argv[0]);
        status = EXIT_FAILURE;
        goto cleanup;
    }

    for (size_t index = operation_count; index > 0; --index) {
        if (process_operation(&operations[index - 1]) == -1) {
            status = EXIT_FAILURE;
        }
    }

cleanup:
    for (size_t index = 0; index < operation_count; ++index) {
        free(operations[index].argument);
    }
    free(operations);
    return status;
}
