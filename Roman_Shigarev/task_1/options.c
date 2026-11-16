#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/resource.h>

extern char **environ;

typedef struct {
    int code;
    char *arg;
} Option;

int main(int argc, char *argv[]) {
    Option *options = NULL;
    size_t count = 0;
    int opt, status = 1;

    if (argc == 1) {
        puts("Options: -i -s -p -u -Ucount -c -Cbytes -d -v -Vname=value");
        return 0;
    }

    opterr = 0;
    while ((opt = getopt(argc, argv, ":ispuU:cC:dvV:")) != -1) {
        if (opt == '?' || opt == ':') {
            fprintf(stderr, "%s: -%c\n",
                    opt == '?' ? "Unknown option" : "Missing argument", optopt);
            goto done;
        }
        Option *temp = realloc(options, (count + 1) * sizeof(*options));
        if (temp == NULL) { perror("realloc"); goto done; }
        options = temp;
        options[count].code = opt;
        options[count].arg = optarg;
        count++;
    }
    if (optind < argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        goto done;
    }

    while (count > 0) {
        Option current = options[--count];
        switch (current.code) {
        case 'i':
            printf("UID=%lu EUID=%lu GID=%lu EGID=%lu\n",
                   (unsigned long)getuid(), (unsigned long)geteuid(),
                   (unsigned long)getgid(), (unsigned long)getegid());
            break;
        case 's':
            if (setpgid(0, 0) == -1) { perror("setpgid"); goto done; }
            break;
        case 'p':
            printf("PID=%ld PPID=%ld PGID=%ld\n",
                   (long)getpid(), (long)getppid(), (long)getpgrp());
            break;
        case 'u':
        case 'c': {
            struct rlimit limit;
            int resource = current.code == 'u' ? RLIMIT_NOFILE : RLIMIT_CORE;
            if (getrlimit(resource, &limit) == -1) {
                perror("getrlimit"); goto done;
            }
            printf("%s=", current.code == 'u' ? "ulimit" : "core");
            if (limit.rlim_cur == RLIM_INFINITY) puts("unlimited");
            else printf("%llu\n", (unsigned long long)limit.rlim_cur);
            break;
        }
        case 'U':
        case 'C': {
            struct rlimit limit;
            char *end;
            int resource = current.code == 'U' ? RLIMIT_NOFILE : RLIMIT_CORE;
            errno = 0;
            long value = strtol(current.arg, &end, 10);
            if (errno || current.arg[0] < '0' || current.arg[0] > '9' || *end) {
                fprintf(stderr, "Invalid number: %s\n", current.arg);
                goto done;
            }
            if (getrlimit(resource, &limit) == -1) {
                perror("getrlimit"); goto done;
            }
            limit.rlim_cur = (rlim_t)value;
            if (setrlimit(resource, &limit) == -1) {
                perror("setrlimit"); goto done;
            }
            break;
        }
        case 'd': {
            char path[PATH_MAX];
            if (getcwd(path, sizeof(path)) == NULL) {
                perror("getcwd"); goto done;
            }
            puts(path);
            break;
        }
        case 'v':
            for (char **entry = environ; *entry; entry++) puts(*entry);
            break;
        case 'V': {
            char *equals = strchr(current.arg, '=');
            if (equals == NULL || equals == current.arg) {
                fputs("Use -Vname=value\n", stderr);
                goto done;
            }
            *equals = '\0';
            int result = setenv(current.arg, equals + 1, 1);
            *equals = '=';
            if (result == -1) { perror("setenv"); goto done; }
            break;
        }
        }
    }
    status = 0;
done:
    free(options);
    return status;
}
