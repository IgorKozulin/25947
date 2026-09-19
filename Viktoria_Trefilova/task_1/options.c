#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __sun
#include <ulimit.h>
#endif

extern char **environ;
static int set_limit(const char *name, int resource, const char *text);

typedef struct {
    int option;
    const char *argument;
} action_t;

static void print_limit(const char *name, int resource) {
    struct rlimit lim;

    if (getrlimit(resource, &lim) == -1) {
        perror("getrlimit");
        return;
    }

    if (lim.rlim_cur == RLIM_INFINITY)
        printf("%s: unlimited\n", name);
    else
        printf("%s: %llu bytes\n", name,
               (unsigned long long)lim.rlim_cur);
}

/* Solaris keeps the historical ulimit(3C) interface used in the assignment. */
static void print_file_size_limit(void) {
#ifdef __sun
    long blocks = ulimit(UL_GETFSIZE);

    if (blocks == -1L) {
        perror("ulimit(UL_GETFSIZE)");
        return;
    }

    printf("file size limit: %ld blocks (512 bytes each)\n", blocks);
#else
    print_limit("file size limit", RLIMIT_FSIZE);
#endif
}

static int set_file_size_limit(const char *text) {
#ifdef __sun
    char *end;
    long blocks;

    errno = 0;
    blocks = strtol(text, &end, 10);

    if (errno || *text == '\0' || *end != '\0' || blocks < 0) {
        fprintf(stderr, "-U: incorrect number of 512-byte blocks: %s\n",
                text);
        return -1;
    }

    if (ulimit(UL_SETFSIZE, blocks) == -1L) {
        perror("ulimit(UL_SETFSIZE)");
        return -1;
    }

    return 0;
#else
    return set_limit("-U", RLIMIT_FSIZE, text);
#endif
}

static int set_limit(const char *name, int resource, const char *text) {
    char *end;
    unsigned long long value;
    struct rlimit lim;

    errno = 0;
    value = strtoull(text, &end, 10);

    if (errno || *text == '\0' || *end != '\0') {
        fprintf(stderr, "%s: incorrect numeric value: %s\n", name, text);
        return -1;
    }

    if (getrlimit(resource, &lim) == -1) {
        perror("getrlimit");
        return -1;
    }

    lim.rlim_cur = (rlim_t)value;

    if (setrlimit(resource, &lim) == -1) {
        perror("setrlimit");
        return -1;
    }

    return 0;
}

static int set_environment(const char *text) {
    const char *equal = strchr(text, '=');
    char *name;
    int result;

    if (equal == NULL || equal == text) {
        fprintf(stderr, "-V requires NAME=VALUE\n");
        return -1;
    }

    name = strndup(text, (size_t)(equal - text));

    if (name == NULL) {
        perror("strndup");
        return -1;
    }

    result = setenv(name, equal + 1, 1);

    if (result == -1)
        perror("setenv");

    free(name);
    return result;
}

static void usage(const char *program) {
#ifdef __sun
    fprintf(stderr,
            "Usage: %s [-i] [-s] [-p] [-u] [-U blocks] "
            "[-c] [-C bytes] [-d] [-v] [-V name=value]\n",
            program);
#else
    fprintf(stderr,
            "Usage: %s [-i] [-s] [-p] [-u] [-U bytes] "
            "[-c] [-C bytes] [-d] [-v] [-V name=value]\n",
            program);
#endif
}

int main(int argc, char **argv) {
    action_t actions[128];
    int count = 0;
    int opt;
    int i;

    opterr = 0;

    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        if (opt == '?') {
            fprintf(stderr, "Unknown option: -%c\n",
                    optopt ? optopt : '?');
            usage(argv[0]);
            return EXIT_FAILURE;
        }

        if (count == (int)(sizeof actions / sizeof actions[0])) {
            fputs("Too many options\n", stderr);
            return EXIT_FAILURE;
        }

        actions[count++] = (action_t){opt, optarg};
    }

    if (optind != argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        return EXIT_FAILURE;
    }

    /* The assignment requires applying options from right to left. */
    for (i = count - 1; i >= 0; --i) {
        switch (actions[i].option) {
        case 'i':
            printf("real uid=%u, effective uid=%u, "
                   "real gid=%u, effective gid=%u\n",
                   getuid(), geteuid(), getgid(), getegid());
            break;

        case 's':
            if (setpgid(0, 0) == -1)
                perror("setpgid");
            break;

        case 'p':
            printf("pid=%d, ppid=%d, pgid=%d\n",
                   getpid(), getppid(), getpgrp());
            break;

        case 'u':
            print_file_size_limit();
            break;

        case 'U':
            set_file_size_limit(actions[i].argument);
            break;

        case 'c':
            print_limit("core file size limit", RLIMIT_CORE);
            break;

        case 'C':
            set_limit("-C", RLIMIT_CORE, actions[i].argument);
            break;

        case 'd': {
            char cwd[PATH_MAX];

            if (getcwd(cwd, sizeof cwd))
                puts(cwd);
            else
                perror("getcwd");
            break;
        }

        case 'v':
            for (char **p = environ; *p; ++p)
                puts(*p);
            break;

        case 'V':
            set_environment(actions[i].argument);
            break;
        }
    }

    return EXIT_SUCCESS;
}
