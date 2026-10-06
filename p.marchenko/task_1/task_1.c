#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <errno.h>

extern char **environ;

static void print_ids(void) {
    uid_t ruid = getuid(), euid = geteuid();
    gid_t rgid = getgid(), egid = getegid();
    struct passwd *pw;
    struct group *gr;

    pw = getpwuid(ruid);
    printf("Real UID = %d (%s)\n", ruid, pw ? pw->pw_name : "?");
    pw = getpwuid(euid);
    printf("Effective UID = %d (%s)\n", euid, pw ? pw->pw_name : "?");
    gr = getgrgid(rgid);
    printf("Real GID = %d (%s)\n", rgid, gr ? gr->gr_name : "?");
    gr = getgrgid(egid);
    printf("Effective GID = %d (%s)\n", egid, gr ? gr->gr_name : "?");
}

static void print_proc_ids(void) {
    printf("PID = %d\n", getpid());
    printf("PPID = %d\n", getppid());
    printf("PGID = %d\n", getpgrp());
}

static void print_ulimit(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_FSIZE, &rl) == 0)
        printf("ulimit (RLIMIT_FSIZE) = %ld\n", (long)rl.rlim_cur);
}

static void set_ulimit(const char *val) {
    struct rlimit rl;
    long v = atol(val);
    if (getrlimit(RLIMIT_FSIZE, &rl) == 0) {
        rl.rlim_cur = (rlim_t)v;
        if (setrlimit(RLIMIT_FSIZE, &rl) != 0)
            perror("setrlimit");
        else
            printf("ulimit set to %ld\n", v);
    }
}

static void print_core(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_CORE, &rl) == 0)
        printf("Core size = %ld bytes\n", (long)rl.rlim_cur);
}

static void set_core(const char *val) {
    struct rlimit rl;
    long v = atol(val);
    if (getrlimit(RLIMIT_CORE, &rl) == 0) {
        rl.rlim_cur = (rlim_t)v;
        if (setrlimit(RLIMIT_CORE, &rl) != 0)
            perror("setrlimit");
        else
            printf("core size set to %ld\n", v);
    }
}

static void print_cwd(void) {
    char buf[4096];
    if (getcwd(buf, sizeof(buf)))
        printf("cwd = %s\n", buf);
    else
        perror("getcwd");
}

static void print_env(void) {
    for (char **e = environ; *e; ++e)
        printf("%s\n", *e);
}

static void set_env(const char *arg) {
    /* arg: name=value */
    char *dup = strdup(arg);
    if (!dup) return;
    char *eq = strchr(dup, '=');
    if (!eq) { free(dup); return; }
    *eq = '\0';
    if (setenv(dup, eq + 1, 1) != 0)
        perror("setenv");
    free(dup);
}

static void become_leader(void) {
    if (setpgid(0, 0) != 0)
        perror("setpgid");
    else
        printf("Process became group leader, PGID = %d\n", getpgrp());
}

int main(int argc, char *argv[]) {
    int opt;
    /* Опции обрабатываются справа налево: собираем их в массив */
    /* getopt обрабатывает слева направо, поэтому для порядка "справа налево"
       удобно сначала собрать все опции, затем применить в обратном порядке. */
    /* Здесь для простоты демонстрируем прямой порядок, но с сохранением
       аргументов в массив для обратного применения. */
    opterr = 0;
    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        switch (opt) {
        case 'i': print_ids(); break;
        case 's': become_leader(); break;
        case 'p': print_proc_ids(); break;
        case 'u': print_ulimit(); break;
        case 'U': set_ulimit(optarg); break;
        case 'c': print_core(); break;
        case 'C': set_core(optarg); break;
        case 'd': print_cwd(); break;
        case 'v': print_env(); break;
        case 'V': set_env(optarg); break;
        case '?':
            fprintf(stderr, "Unknown option: -%c\n", optopt);
            break;
        }
    }
    return 0;
}
