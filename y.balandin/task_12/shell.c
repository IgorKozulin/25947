#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include "shell.h"

/* Глобальные переменные (extern в shell.h) */
char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char *argv[])
{
    int i;
    char line[1024];      /* буфер для строки */
    int ncmds;            /* число разобранных команд */
    char prompt[50];      /* приглашение */

    /* PLACE SIGNAL CODE HERE */

    sprintf(prompt, "[%s] ", argv[0]);

    /* Главный цикл: читаем строку, пока не Ctrl+D */
    while (promptline(prompt, line, sizeof(line)) > 0) {

        /* Разбор строки; 0 — пусто, -1 — ошибка */
        if ((ncmds = parseline(line)) <= 0)
            continue;

#ifdef DEBUG
        /* Отладочный вывод разбора (собирается с -DDEBUG) */
        {
            int i, j;
            for (i = 0; i < ncmds; i++) {
                for (j = 0; cmds[i].cmdargs[j] != (char *) NULL; j++)
                    fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n",
                            i, j, cmds[i].cmdargs[j]);
                fprintf(stderr, "cmds[%d].cmdflag = %o\n", i,
                        cmds[i].cmdflag);
            }
            if (infile)
                fprintf(stderr, "infile = %s\n", infile);
            if (outfile)
                fprintf(stderr, "outfile = %s\n", outfile);
            if (appfile)
                fprintf(stderr, "appfile = %s\n", appfile);
            fprintf(stderr, "bkgrnd = %d\n", bkgrnd);
        }
#endif

        /* Место для fork/exec — в задаче 13 */
        for (i = 0; i < ncmds; i++) {
            /* FORK AND EXECUTE */
        }
    }

    return 0;
}
