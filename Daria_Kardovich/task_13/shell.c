#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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

        for (int i = 0; i < ncmds; i++) {
            pid_t pid;
            int status;

            pid = fork();

            if (pid == -1) {
                perror("fork");
                continue;
            }

            if (pid == 0) {
                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);

                perror(cmds[i].cmdargs[0]);
                _exit(127);
            }

            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                continue;
            }

            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                fprintf(stderr, "Command exited with status %d\n",
                        WEXITSTATUS(status));
            }

            if (WIFSIGNALED(status)) {
                fprintf(stderr, "Command terminated by signal %d\n",
                        WTERMSIG(status));
            }
        }
    }

    return 0;
}
