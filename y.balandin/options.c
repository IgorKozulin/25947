#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/resource.h>
#include <sys/types.h>

extern char **environ;

struct opt_rec {
    int opt;      /* символ опции */
    char *arg;      /* optarg или NULL */
};

/* ---------- Реализации опций ---------- */

static void do_i(void) {
    printf("-i: real UID=%d, effective UID=%d, real GID=%d, effective GID=%d\n",
           (int)getuid(), (int)geteuid(),
           (int)getgid(), (int)getegid());
}

static void do_s(void) {
    if (setpgid(0, 0) == -1) {
        perror("-s: setpgid");
    } else {
        printf("-s: process is now a group leader, PGID=%d\n",
               (int)getpgid(0));
    }
}

static void do_p(void) {
    printf("-p: PID=%d, PPID=%d, PGID=%d\n",
           (int)getpid(), (int)getppid(), (int)getpgid(0));
}

static void do_u(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == -1) {
        perror("-u: getrlimit");
        return;
    }
    printf("-u: ulimit (RLIMIT_NOFILE): soft=%llu, hard=%llu\n",
           (unsigned long long)rl.rlim_cur,
           (unsigned long long)rl.rlim_max);
}

static void do_U(const char *arg) {
    if (!arg) { fprintf(stderr, "-U: missing argument\n"); return; }

    errno = 0;
    char *end = NULL;
    long val = strtol(arg, &end, 10);

    if (errno != 0 || end == arg || *end != '\0' || val < 0) {
        fprintf(stderr, "-U: invalid value '%s'\n", arg);
        return;
    }

    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == -1) {
        perror("-U: getrlimit");
        return;
    }
    rl.rlim_cur = (rlim_t)val;
    /* не даём выйти за hard limit */
    if (rl.rlim_max != RLIM_INFINITY && rl.rlim_cur > rl.rlim_max)
        rl.rlim_max = rl.rlim_cur;

    if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
        perror("-U: setrlimit");
    } else {
        printf("-U: ulimit (RLIMIT_NOFILE) changed to %ld\n", val);
    }
}

static void do_c(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("-c: getrlimit");
        return;
    }
    printf("-c: core file size: soft=%llu, hard=%llu bytes\n",
           (unsigned long long)rl.rlim_cur,
           (unsigned long long)rl.rlim_max);
}

static void do_C(const char *arg) {
    if (!arg) { fprintf(stderr, "-C: missing argument\n"); return; }

    errno = 0;
    char *end = NULL;
    long val = strtol(arg, &end, 10);

    if (errno != 0 || end == arg || *end != '\0' || val < 0) {
        fprintf(stderr, "-C: invalid value '%s'\n", arg);
        return;
    }

    struct rlimit rl;
    if (getrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("-C: getrlimit");
        return;
    }
    rl.rlim_cur = (rlim_t)val;
    if (rl.rlim_max != RLIM_INFINITY && rl.rlim_cur > rl.rlim_max)
        rl.rlim_max = rl.rlim_cur;

    if (setrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("-C: setrlimit");
    } else {
        printf("-C: core file size changed to %ld bytes\n", val);
    }
}

static void do_d(void) {
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf)) == NULL) {
        perror("-d: getcwd");
        return;
    }
    printf("-d: current directory: %s\n", buf);
}

static void do_v(void) {
    printf("-v: environment variables:\n");
    for (char **ep = environ; ep && *ep; ep++)
        printf("    %s\n", *ep);
}

static void do_V(const char *arg) {
    if (!arg || strchr(arg, '=') == NULL) {
        fprintf(stderr, "-V: expected NAME=value, got '%s'\n",
                arg ? arg : "(null)");
        return;
    }
    char *copy = strdup(arg);
    if (!copy) { perror("-V: strdup"); return; }
    if (putenv(copy) != 0) {
        perror("-V: putenv");
        free(copy);
    } else {
        printf("-V: environment variable set: %s\n", arg);
    }
}

int main(int argc, char *argv[]) {
    const char *optstring = "ispuU:cC:dvV:";
    int c;

    size_t cap = 16, n = 0;
    struct opt_rec *recs = malloc(cap * sizeof(*recs));
    if (!recs) { perror("malloc"); return 1; }

    /* Фаза 1: только парсим, ничего не выполняем */
    while ((c = getopt(argc, argv, optstring)) != -1) {
        if (n == cap) {
            cap *= 2;
            struct opt_rec *tmp = realloc(recs, cap * sizeof(*recs));
            if (!tmp) { perror("realloc"); free(recs); return 1; }
            recs = tmp;
        }
        if (c == '?') {
            /* getopt уже напечатал сообщение; optopt содержит символ */
            fprintf(stderr, "Unknown option: -%c\n",
                    optopt ? optopt : '?');
            free(recs);
            return 1;
        }
        recs[n].opt = c;
        recs[n].arg = optarg;   /* optarg жив до конца main */
        n++;
    }

    /* Фаза 2: выполняем в обратном порядке (справа налево) */
    for (size_t i = n; i-- > 0; ) {
        switch (recs[i].opt) {
            case 'i': do_i();        break;
            case 's': do_s();        break;
            case 'p': do_p();        break;
            case 'u': do_u();        break;
            case 'U': do_U(recs[i].arg); break;
            case 'c': do_c();        break;
            case 'C': do_C(recs[i].arg); break;
            case 'd': do_d();        break;
            case 'v': do_v();        break;
            case 'V': do_V(recs[i].arg); break;
        }
    }

    free(recs);
    return 0;
}
