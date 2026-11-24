#include <stdio.h>
#include <sys/types.h> // Добавлено для типа pid_t
#include <sys/wait.h> // Добавлено для waitpid и макросов статуса
#include <unistd.h> // Добавлено для fork execvp и _exit

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

        for (int i = 0; i < ncmds; i++) { // Добавлено в задании 13 для запуска каждой команды
            pid_t pid; // Хранит номер дочернего процесса
            int status; // Хранит статус завершения дочернего процесса

            pid = fork(); // Создаёт дочерний процесс

            if (pid == -1) { // Проверяет ошибку создания процесса
                perror("fork"); // Печатает причину ошибки fork
                continue; // Переходит к следующей команде
            }

            if (pid == 0) { // Этот блок выполняется только в дочернем процессе
                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs); // Запускает введённую команду

                perror(cmds[i].cmdargs[0]); // Печатает ошибку если команда не найдена
                _exit(127); // Завершает потомка с кодом ошибки 127
            }

            if (waitpid(pid, &status, 0) == -1) { // Родительский shell ждёт завершения потомка
                perror("waitpid"); // Печатает причину ошибки ожидания
                continue; // Переходит к следующей команде
            }

            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) { // Проверяет завершение команды с ненулевым кодом
                fprintf(stderr, "Command exited with status %d\n",
                        WEXITSTATUS(status)); // Печатает код завершения команды
            }

            if (WIFSIGNALED(status)) { // Проверяет завершение команды из-за сигнала
                fprintf(stderr, "Command terminated by signal %d\n",
                        WTERMSIG(status)); // Печатает номер сигнала
            }
        }
    }

    return 0;
}
