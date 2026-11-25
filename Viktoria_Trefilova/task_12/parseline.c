#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"

static char *blankskip(char *s)
{
    while (isspace((unsigned char)*s))
        ++s;
    return s;
}

int parseline(char *line)
{
    static const char delimiters[] = " \t\r\n\v\f|&<>;";
    int current = 0;
    int nargs = 0;
    int ncmds = 0;
    char *s = line;
    char *word_end = NULL;

    infile = outfile = appfile = NULL;
    bkgrnd = 0;
    for (int i = 0; i < MAXCMDS; ++i) {
        cmds[i].cmdargs[0] = NULL;
        cmds[i].cmdflag = 0;
    }

    while (*(s = blankskip(s))) {
        char symbol = *s;

        /* Save the next symbol before terminating the preceding word. */
        if (s == word_end) {
            *s = '\0';
            word_end = NULL;
        }

        switch (symbol) {
        case '&':
            bkgrnd = 1;
            ++s;
            break;

        case '<':
        case '>': {
            char **file = &infile;

            ++s;
            if (symbol == '>') {
                file = &outfile;
                if (*s == '>') {
                    file = &appfile;
                    ++s;
                }
            }

            s = blankskip(s);
            if (*s == '\0' || strchr(delimiters, *s))
                goto syntax_error;
            *file = s;
            s += strcspn(s, delimiters);
            word_end = s;
            break;
        }

        case '|':
            if (nargs == 0 || current + 1 >= MAXCMDS)
                goto syntax_error;
            cmds[current++].cmdflag |= OUTPIP;
            cmds[current].cmdflag |= INPIP;
            nargs = 0;
            ++s;
            break;

        case ';':
            if (nargs > 0)
                ++current;
            else if (current < MAXCMDS && (cmds[current].cmdflag & INPIP))
                goto syntax_error;
            nargs = 0;
            ++s;
            break;

        default:
            if (current >= MAXCMDS || nargs >= MAXARGS - 1)
                goto syntax_error;
            cmds[current].cmdargs[nargs++] = s;
            cmds[current].cmdargs[nargs] = NULL;
            ncmds = current + 1;
            s += strcspn(s, delimiters);
            word_end = s;
            break;
        }

        if (s == word_end && isspace((unsigned char)*s)) {
            *s++ = '\0';
            word_end = NULL;
        }
    }

    if (current < MAXCMDS && (cmds[current].cmdflag & INPIP) && nargs == 0)
        goto syntax_error;

    return ncmds;

syntax_error:
    fprintf(stderr, "syntax error\n");
    return -1;
}
