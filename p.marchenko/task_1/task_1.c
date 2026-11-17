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
#include <time.h>

extern char **environ;

static void do_i(void) {
    uid_t ruid = getuid(), euid = geteuid();
    gid_t rgid = getgid(), egid = getegid();
    struct passwd *pw; struct group *gr;
    pw = getpwuid(ruid);
    printf("Real UID = %d (%s)\n", ruid, pw ? pw->pw_name : "?");
    pw = getpwuid(euid);
    printf("Effective UID = %d (%s)\n", euid, pw ? pw->pw_name : "?");
    gr = getgrgid(rgid);
    printf("Real GID = %d (%s)\n", rgid, gr ? gr->gr_name : "?");
    gr = getgrgid(egid);
    printf("Effective GID = %d (%s)\n", egid, gr ? gr->gr_name : "?");
}
static void do_p(void) {
    printf("PID = %d\n", getpid());
    printf("PPID = %d\n", getppid());
    printf("PGID = %d\n", getpgrp());
}
static void do_s(void) {
    if (setpgid(0, 0) != 0) perror("setpgid");
    else printf("Process became group leader, PGID = %d\n", getpgrp());
}
static void do_u(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_FSIZE, &rl) == 0)
        printf("ulimit = %ld\n", (long)rl.rlim_cur);
}
static void do_U(const char *v) {
    struct rlimit rl; long x = atol(v);
    if (getrlimit(RLIMIT_FSIZE, &rl) == 0) {
        rl.rlim_cur = (rlim_t)x;
        if (setrlimit(RLIMIT_FSIZE, &rl) != 0) perror("setrlimit");
        else printf("ulimit set to %ld\n", x);
    }
}
static void do_c(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_CORE, &rl) == 0)
        printf("Core size = %ld\n", (long)rl.rlim_cur);
}
static void do_C(const char *v) {
    struct rlimit rl; long x = atol(v);
    if (getrlimit(RLIMIT_CORE, &rl) == 0) {
        rl.rlim_cur = (rlim_t)x;
        if (setrlimit(RLIMIT_CORE, &rl) != 0) perror("setrlimit");
        else printf("core size set to %ld\n", x);
    }
}
static void do_d(void) {
    char buf[4096];
    if (getcwd(buf, sizeof(buf))) printf("cwd = %s\n", buf);
    else perror("getcwd");
}
static void do_v(void) {
    for (char **e = environ; *e; ++e) printf("%s\n", *e);
}
static void do_V(const char *arg) {
    char *dup = strdup(arg);
    if (!dup) return;
    char *eq = strchr(dup, '=');
    if (!eq) { free(dup); return; }
    *eq = '\0';
    if (setenv(dup, eq + 1, 1) != 0) perror("setenv");
    free(dup);
}
static void do_t(void) {
    time_t t = time(NULL);
    t -= 8 * 3600;                 /* PST = UTC-8 */
    struct tm *tm = gmtime(&t);
    char buf[128];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S PST", tm);
    printf("%s\n", buf);
}

struct action { int opt; char *arg; };

int main(int argc, char *argv[]) {
    struct action acts[256];
    int n = 0;
    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:t")) != -1) {
        acts[n].opt = opt;
        acts[n].arg = optarg;
        n++;
    }
    /* применяем СПРАВА НАЛЕВО */
    for (int k = n - 1; k >= 0; --k) {
        switch (acts[k].opt) {
        case 'i': do_i(); break;
        case 's': do_s(); break;
        case 'p': do_p(); break;
        case 'u': do_u(); break;
        case 'U': do_U(acts[k].arg); break;
        case 'c': do_c(); break;
        case 'C': do_C(acts[k].arg); break;
        case 'd': do_d(); break;
        case 'v': do_v(); break;
        case 'V': do_V(acts[k].arg); break;
        case 't': do_t(); break;
        case '?':
            fprintf(stderr, "Unknown option: -%c\n", optopt);
            break;
        }
    }
    return 0;
}
