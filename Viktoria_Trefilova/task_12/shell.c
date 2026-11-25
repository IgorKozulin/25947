#include <stdio.h>

#include "shell.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char *argv[])
{
    char line[1024];
    char prompt[50];
    int length;

    snprintf(prompt, sizeof(prompt), "[%s] ", argc > 0 ? argv[0] : "myshell");

    while ((length = promptline(prompt, line, sizeof(line))) > 0) {
        int ncmds = parseline(line);
        if (ncmds <= 0)
            continue;

#ifdef DEBUG
        for (int i = 0; i < ncmds; ++i) {
            for (int j = 0; cmds[i].cmdargs[j] != NULL; ++j)
                fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n",
                        i, j, cmds[i].cmdargs[j]);
            fprintf(stderr, "cmds[%d].cmdflag = %o\n",
                    i, (unsigned int)cmds[i].cmdflag);
        }

        if (infile)
            fprintf(stderr, "infile  = %s\n", infile);
        if (outfile)
            fprintf(stderr, "outfile = %s\n", outfile);
        if (appfile)
            fprintf(stderr, "appfile = %s\n", appfile);
        fprintf(stderr, "bkgrnd = %d\n", bkgrnd);
#endif

        /* FORK AND EXECUTE */
    }

    return length < 0 ? 1 : 0;
}
