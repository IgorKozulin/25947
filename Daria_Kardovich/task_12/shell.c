#include <stdio.h>

#include "shell.h"

char *infile;
char *outfile;
char *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

#ifdef DEBUG
static void print_parsed_commands(int ncmds)
{
    for (int i = 0; i < ncmds; i++) {
        for (int j = 0; cmds[i].cmdargs[j] != NULL; j++) {
            fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n",
                    i, j, cmds[i].cmdargs[j]);
        }

        fprintf(stderr, "cmds[%d].cmdflag = %o\n",
                i, cmds[i].cmdflag);
    }

    fprintf(stderr, "bkgrnd = %d\n", bkgrnd);

    if (infile != NULL) {
        fprintf(stderr, "infile = %s\n", infile);
    }

    if (outfile != NULL) {
        fprintf(stderr, "outfile = %s\n", outfile);
    }

    if (appfile != NULL) {
        fprintf(stderr, "appfile = %s\n", appfile);
    }
}
#endif

int main(int argc, char *argv[])
{
    char line[1024];
    char prompt[50];
    int ncmds;

    (void)argc;

    snprintf(prompt, sizeof(prompt), "[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) {
        ncmds = parseline(line);

        if (ncmds <= 0) {
            continue;
        }

#ifdef DEBUG
        print_parsed_commands(ncmds);
#endif
    }

    return 0;
}
