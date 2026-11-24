
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"

static char *blankskip(char *s){
    while (*s && isspace((unsigned char)*s)){
        s++;
    }
    return s;
}

int parseline(char *line){
    int nargs = 0;
    int ncmds = 0;
    int rval = 0;
    char *s = line;
    char *end;
    char delim[] = " \t|&<>;\n";

    bkgrnd = 0;
    infile = outfile = appfile = NULL;

    for (int i = 0; i < MAXCMDS; i++){
        cmds[i].cmdflag = 0;
        cmds[i].cmdargs[0] = NULL;
    }

    while (*s){
        s = blankskip(s);

        if (!*s){
            break;
        }

        switch (*s){
            case '&':
                bkgrnd = 1;
                *s++ = '\0';
                break;

            case '>':
            {
                int append = 0;

                if (*(s + 1) == '>'){
                    append = 1;
                    *s++ = '\0';
                }

                *s++ = '\0';
                s = blankskip(s);

                if (!*s){
                    fprintf(stderr, "syntax error\n");
                    return -1;
                }

                end = strpbrk(s, delim);

                if (append){
                    appfile = s;
                }else{
                    outfile = s;
                }

                if (end){
                    char separator = *end;
                    *end = '\0';
                    s = end + 1;

                    if (!isspace((unsigned char)separator)){
                        fprintf(stderr, "syntax error\n");
                        return -1;
                    }
                }else{
                    s += strlen(s);
                }

                break;
            }

            case '<':
                *s++ = '\0';
                s = blankskip(s);

                if (!*s){
                    fprintf(stderr, "syntax error\n");
                    return -1;
                }

                infile = s;
                end = strpbrk(s, delim);

                if (end){
                    char separator = *end;
                    *end = '\0';
                    s = end + 1;

                    if (!isspace((unsigned char)separator)){
                        fprintf(stderr, "syntax error\n");
                        return -1;
                    }
                }else{
                    s += strlen(s);
                }
                break;

            case '|':
                if (nargs == 0 || ncmds + 1 >= MAXCMDS){
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

                if (nargs > 0){
                    ncmds++;
                }

                nargs = 0;
                break;

            default:
                if (ncmds >= MAXCMDS || nargs >= MAXARGS - 1){
                    fprintf(stderr, "syntax error\n");
                    return -1;
                }

                if (nargs == 0){
                    rval = ncmds + 1;
                }

                cmds[ncmds].cmdargs[nargs++] = s;
                cmds[ncmds].cmdargs[nargs] = NULL;

                end = strpbrk(s, delim);

                if (end){
                    if (isspace((unsigned char)*end)){
                        *end = '\0';
                        s = end + 1;
                    }else{
                        s = end;
                    }
                }else{
                    s += strlen(s);
                }
                break;
        }
    }

    if (ncmds > 0 && (cmds[ncmds - 1].cmdflag & OUTPIP)
        && nargs == 0){
        fprintf(stderr, "syntax error\n");
        return -1;
    }

    return rval;
}

