#ifndef SHELL_H
#define SHELL_H

#define MAXARGS 256
#define MAXCMDS 50

struct command {
    char *cmdargs[MAXARGS];
    char cmdflag;
};

#define OUTPIP 01
#define INPIP 02

extern struct command cmds[];
extern char *infile;
extern char *outfile;
extern char *appfile;
extern char bkgrnd;

int parseline(char *);
int promptline(char *, char *, int);

#endif
