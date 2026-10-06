#ifndef QUERY_H
#define QUERY_H

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t time_expired;

static inline void alarm_handler(int signum)
{
    (void)signum;
    time_expired = 1;
}

/* 0: number, 1: EOF, 2: timeout, 3: invalid input, -1: I/O error. */
static inline int query_line_number(int timed, long *number)
{
    char input[64];
    size_t used = 0;
    int ch;
    int too_long = 0;
    char *end;

    printf("Line number (0 to quit): ");
    if (fflush(stdout) == EOF)
        return -1;
    time_expired = 0;
    if (timed)
        alarm(5);
    for (;;) {
        errno = 0;
        ch = getchar();
        if (time_expired) {
            alarm(0);
            clearerr(stdin);
            return 2;
        }
        if (ch == EOF) {
            if (ferror(stdin)) {
                alarm(0);
                return -1;
            }
            if (used == 0) {
                alarm(0);
                return 1;
            }
            break;
        }
        if (ch == '\n')
            break;
        if (used + 1 < sizeof(input))
            input[used++] = (char)ch;
        else
            too_long = 1;
    }
    if (timed)
        alarm(0);
    input[used] = '\0';
    if (too_long || used == 0)
        return 3;
    errno = 0;
    *number = strtol(input, &end, 10);
    if (errno == ERANGE || *end != '\0' || *number < 0)
        return 3;
    return 0;
}

static inline int install_alarm_handler(void)
{
    struct sigaction action = {0};
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    return sigaction(SIGALRM, &action, NULL);
}

#endif
