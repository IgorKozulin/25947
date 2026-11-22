#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"

static char *blankskip(char *s)
{
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }

    return s;
}

int parseline(char *line)
{
    int nargs;
    int ncmds;
    int rval;
    int i;
    char aflg = 0;
    char *s = line;
    static char delim[] = " \t|&<>;\n";

    bkgrnd = 0;
    nargs = 0;
    ncmds = 0;
    rval = 0;

    infile = NULL;
    outfile = NULL;
    appfile = NULL;

    for (i = 0; i < MAXCMDS; i++) {
        cmds[i].cmdflag = 0;
        cmds[i].cmdargs[0] = NULL;
    }

    while (*s) {
        s = blankskip(s);

        if (!*s) {
            break;
        }

        switch (*s) {
            case '&':
                bkgrnd = 1;
                *s++ = '\0';
                break;

            case '>':
                if (*(s + 1) == '>') {
                    aflg = 1;
                    *s++ = '\0';
                }

                *s++ = '\0';
                s = blankskip(s);

                if (!*s) {
                    fprintf(stderr, "syntax error\n");
                    return -1;
                }

                if (aflg) {
                    appfile = s;
                } else {
                    outfile = s;
                }

                s = strpbrk(s, delim);

                if (s != NULL && isspace((unsigned char)*s)) {
                    *s++ = '\0';
                }

                break;

            case '<':
                *s++ = '\0';
                s = blankskip(s);

                if (!*s) {
                    fprintf(stderr, "syntax error\n");
                    return -1;
                }

                infile = s;
                s = strpbrk(s, delim);

                if (s != NULL && isspace((unsigned char)*s)) {
                    *s++ = '\0';
                }

                break;

            case '|':
                if (nargs == 0) {
                    fprintf(stderr, "syntax error\n");
                    return -1;
                }

                cmds[ncmds++].cmdflag |= OUTPIP;
                cmds[ncmds].cmdflag |= INPIP;
                *s++ = '\0';
                nargs = 0;
                break;

            case ';':
                *s++ = '\0';
                ncmds++;
                nargs = 0;
                break;

            default:
                if (nargs == 0) {
                    rval = ncmds + 1;
                }

                cmds[ncmds].cmdargs[nargs++] = s;
                cmds[ncmds].cmdargs[nargs] = NULL;

                s = strpbrk(s, delim);

                if (s != NULL && isspace((unsigned char)*s)) {
                    *s++ = '\0';
                }

                break;
        }
    }

    if (ncmds > 0 && nargs == 0 &&
        (cmds[ncmds - 1].cmdflag & OUTPIP)) {
        fprintf(stderr, "syntax error\n");
        return -1;
    }

    return rval;
}
