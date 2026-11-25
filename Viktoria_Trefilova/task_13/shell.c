#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

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

        for (int i = 0; i < ncmds; ++i) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }

            if (pid == 0) {
                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
                perror(cmds[i].cmdargs[0]);
                _exit(127);
            }

            int status;
            pid_t waited;
            do {
                waited = waitpid(pid, &status, 0);
            } while (waited == -1 && errno == EINTR);

            if (waited == -1) {
                perror("waitpid");
            } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                fprintf(stderr, "Command exited with status %d\n",
                        WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "Command terminated by signal\n");
            }
        }
    }

    return length < 0 ? 1 : 0;
}
