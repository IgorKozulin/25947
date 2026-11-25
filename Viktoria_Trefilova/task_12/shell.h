#ifndef SHELL_H
#define SHELL_H

#define MAXARGS 256
#define MAXCMDS 50

#define OUTPIP 01
#define INPIP  02

struct command {
    char *cmdargs[MAXARGS];
    char cmdflag;
};

extern struct command cmds[MAXCMDS];
extern char *infile, *outfile, *appfile;
extern char bkgrnd;

int parseline(char *line);
int promptline(const char *prompt, char *line, int sizline);

#endif
