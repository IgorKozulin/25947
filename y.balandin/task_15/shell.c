#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "shell.h"

/* Глобальные переменные (extern в shell.h) */
char *infile;      /* файл для '<'  */
char *outfile;     /* файл для '>'  */
char *appfile;     /* файл для '>>' */
struct command cmds[MAXCMDS];
char bkgrnd;       /* 1 = фон (&)   */

int main(int argc, char *argv[])
{
    int i;
    char line[1024];      /* буфер для ввода */
    int ncmds;            /* число команд */
    char prompt[50];      /* приглашение */

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

        /* Запускаем каждую команду из строки */
        for (i = 0; i < ncmds; i++) {
            pid_t pid = fork(); // создаем копию процесса
            if (pid < 0) {
                perror("fork");
                continue;
            }

            if (pid == 0) {
                /* ===== ДОЧЕРНИЙ ПРОЦЕСС ===== */

                /* 1. Ввод: '<' применяется ТОЛЬКО к первой команде (i == 0) */
                if (i == 0 && infile != NULL) {
                    int fd = open(infile, O_RDONLY);
                    if (fd == -1) {
                        perror(infile);
                        _exit(1);
                    }
                    /* Делаем fd копией стандартного ввода (0) */
                    if (dup2(fd, STDIN_FILENO) == -1) {
                        perror("dup2 stdin");
                        _exit(1);
                    }
                    close(fd); /* исходный fd больше не нужен */
                }

                /* 2. Вывод: '>' применяется ТОЛЬКО к последней команде */
                if (i == ncmds - 1 && outfile != NULL) {
                    /* O_TRUNC — обнулить файл перед записью */
                    int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        perror(outfile);
                        _exit(1);
                    }
                    if (dup2(fd, STDOUT_FILENO) == -1) {
                        perror("dup2 stdout");
                        _exit(1);
                    }
                    close(fd);
                }

                /* 3. Дописывание: '>>' применяется ТОЛЬКО к последней команде */
                if (i == ncmds - 1 && appfile != NULL) {
                    /* O_APPEND — писать в конец файла */
                    int fd = open(appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd == -1) {
                        perror(appfile);
                        _exit(1);
                    }
                    if (dup2(fd, STDOUT_FILENO) == -1) {
                        perror("dup2 stdout");
                        _exit(1);
                    }
                    close(fd);
                }

                /* Запуск команды; при ошибке — сообщение и код 127 */
                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs); // заменяем процесс другой программой
                perror(cmds[i].cmdargs[0]);
                _exit(127);
            }

            /* ===== РОДИТЕЛЬ ===== */
            /* Ждём завершения только если команда не фоновая */
            if (!bkgrnd) {
                waitpid(pid, NULL, 0);
            }
        }
    }

    return 0;
}
