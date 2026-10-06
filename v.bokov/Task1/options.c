#define _XOPEN_SOURCE 700
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <ulimit.h>
#include <unistd.h>

extern char **environ;

typedef struct {
    int key;
    char *arg;
} Opt;

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-ispuU:cC:dvV:]\n", prog);
}

static int read_num(const char *str, long *num)
{
    char *end;
    long x;

    errno = 0;
    x = strtol(str, &end, 10);
    if (errno == ERANGE || end == str || *end != '\0' || x < 0)
        return -1;
    *num = x;
    return 0;
}

static int read_size(const char *str, rlim_t *num)
{
    char *end;
    uintmax_t x;

    errno = 0;
    x = strtoumax(str, &end, 10);
    if (str[0] == '-' || errno == ERANGE || end == str || *end != '\0' ||
        (uintmax_t)(rlim_t)x != x)
        return -1;
    *num = (rlim_t)x;
    return 0;
}

static int print_dir(void)
{
    size_t len = 256;

    for (;;) {
        char *buf = malloc(len);
        if (buf == NULL) {
            perror("malloc");
            return -1;
        }
        if (getcwd(buf, len) != NULL) {
            puts(buf);
            free(buf);
            return 0;
        }
        free(buf);
        if (errno != ERANGE) {
            perror("getcwd");
            return -1;
        }
        if (len > SIZE_MAX / 2) {
            fprintf(stderr, "getcwd: path is too long\n");
            return -1;
        }
        len *= 2;
    }
}

static int run_opt(const Opt *opt)
{
    struct rlimit lim;
    long num;
    char **env;

    switch (opt->key) {
    case 'i':
        printf("UID: real=%lu effective=%lu\n",
               (unsigned long)getuid(), (unsigned long)geteuid());
        printf("GID: real=%lu effective=%lu\n",
               (unsigned long)getgid(), (unsigned long)getegid());
        return 0;
    case 's':
        if (setpgid(0, 0) == -1) {
            perror("setpgid");
            return -1;
        }
        return 0;
    case 'p':
        printf("PID=%lu PPID=%lu PGID=%lu\n",
               (unsigned long)getpid(), (unsigned long)getppid(),
               (unsigned long)getpgrp());
        return 0;
    case 'u':
        errno = 0;
        num = ulimit(UL_GETFSIZE);
        if (num == -1 && errno != 0) {
            perror("ulimit(UL_GETFSIZE)");
            return -1;
        }
        printf("ulimit=%ld blocks (512 bytes each)\n", num);
        return 0;
    case 'U':
        if (read_num(opt->arg, &num) == -1) {
            fprintf(stderr, "Invalid value for -U: %s\n", opt->arg);
            return -1;
        }
        if (ulimit(UL_SETFSIZE, num) == -1) {
            perror("ulimit(UL_SETFSIZE)");
            return -1;
        }
        return 0;
    case 'c':
        if (getrlimit(RLIMIT_CORE, &lim) == -1) {
            perror("getrlimit(RLIMIT_CORE)");
            return -1;
        }
        if (lim.rlim_cur == RLIM_INFINITY)
            puts("core size=unlimited");
        else
            printf("core size=%" PRIuMAX " bytes\n", (uintmax_t)lim.rlim_cur);
        return 0;
    case 'C':
        if (getrlimit(RLIMIT_CORE, &lim) == -1) {
            perror("getrlimit(RLIMIT_CORE)");
            return -1;
        }
        if (read_size(opt->arg, &lim.rlim_cur) == -1) {
            fprintf(stderr, "Invalid value for -C: %s\n", opt->arg);
            return -1;
        }
        if (setrlimit(RLIMIT_CORE, &lim) == -1) {
            perror("setrlimit(RLIMIT_CORE)");
            return -1;
        }
        return 0;
    case 'd':
        return print_dir();
    case 'v':
        for (env = environ; *env != NULL; ++env)
            puts(*env);
        return 0;
    case 'V': {
        char *eq = strchr(opt->arg, '=');
        if (eq == opt->arg || eq == NULL) {
            fprintf(stderr, "Invalid value for -V (expected NAME=VALUE): %s\n",
                    opt->arg);
            return -1;
        }
        if (putenv(opt->arg) == -1) {
            perror("putenv");
            return -1;
        }
        return 0;
    }
    }
    return -1;
}

int main(int argc, char *argv[])
{
    Opt *opts = NULL;
    size_t n = 0;
    size_t cap = 0;
    size_t i;
    int key;
    int code = EXIT_SUCCESS;

    opterr = 0;
    while ((key = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        Opt *tmp;
        char *arg = NULL;

        if (key == '?') {
            fprintf(stderr, "Unknown option or missing argument: -%c\n", optopt);
            usage(argv[0]);
            code = EXIT_FAILURE;
            goto cleanup;
        }
        if (key == 'U' || key == 'C' || key == 'V') {
            arg = strdup(optarg);
            if (arg == NULL) {
                perror("strdup");
                code = EXIT_FAILURE;
                goto cleanup;
            }
        }
        if (n == cap) {
            size_t new_cap = cap == 0 ? 8 : cap * 2;
            tmp = realloc(opts, new_cap * sizeof(*opts));
            if (tmp == NULL) {
                perror("realloc");
                free(arg);
                code = EXIT_FAILURE;
                goto cleanup;
            }
            opts = tmp;
            cap = new_cap;
        }
        opts[n].key = key;
        opts[n].arg = arg;
        ++n;
    }
    if (optind != argc) {
        fprintf(stderr, "Unexpected operand: %s\n", argv[optind]);
        usage(argv[0]);
        code = EXIT_FAILURE;
        goto cleanup;
    }
    for (i = n; i > 0; --i) {
        if (run_opt(&opts[i - 1]) == -1) {
            code = EXIT_FAILURE;
            break;
        }
    }

cleanup:
    for (i = 0; i < n; ++i) {
        if (opts[i].key != 'V')
            free(opts[i].arg);
    }
    free(opts);
    return code;
}
