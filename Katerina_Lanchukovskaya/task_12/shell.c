
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include "shell.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char *argv[]){
    char line[1024];
    char prompt[50];
    int ncmds;

    snprintf(prompt, sizeof(prompt), "[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0){

        ncmds = parseline(line);

        if (ncmds <= 0){
            continue;
        }

#ifdef DEBUG
        for (int i = 0; i < ncmds; i++){
            for (int j = 0; cmds[i].cmdargs[j] != NULL; j++){
                fprintf(stderr,
                        "cmd[%d].cmdargs[%d] = %s\n",
                        i, j, cmds[i].cmdargs[j]);
            }

            fprintf(stderr,
                    "cmds[%d].cmdflag = %o\n",
                    i, cmds[i].cmdflag);
        }

        if (infile != NULL){
            fprintf(stderr, "infile = %s\n", infile);
        }

        if (outfile != NULL){
            fprintf(stderr, "outfile = %s\n", outfile);
        }

        if (appfile != NULL){
            fprintf(stderr, "appfile = %s\n", appfile);
        }

        fprintf(stderr, "bkgrnd = %d\n", bkgrnd);
#endif
    }

    return 0;
}

