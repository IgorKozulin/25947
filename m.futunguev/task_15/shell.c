#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include "shell.h"
#include <fcntl.h>

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char *argv[])
{
    register int i;
    char line[1024];      /*  allow large command lines  */
    int ncmds;
    char prompt[50];      /* shell prompt */

    /* PLACE SIGNAL CODE HERE */

    sprintf(prompt, "[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) {    /* until eof  */
        while (waitpid(-1, NULL, WNOHANG) > 0); // подбираем завершившиеся фоновые процессы
        
        if ((ncmds = parseline(line)) <= 0)
            continue;   /* read next line */
#ifdef DEBUG
        {
            int i, j;
            for (i = 0; i < ncmds; i++) {
                for (j = 0; cmds[i].cmdargs[j] != (char *) NULL; j++)
                    fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n",
                            i, j, cmds[i].cmdargs[j]);
                fprintf(stderr, "cmds[%d].cmdflag = %o\n", i, cmds[i].cmdflag);
            }
        }
#endif

        for (i = 0; i < ncmds; i++) {
            // «Номер» — это файловый дескриптор: число, по которому программа обращается к открытому файлу.
            pid_t pid = fork(); // создаем копию процесса
            if(pid == -1){perror("fork"); continue;} // копию создать не удалось
            if (pid == 0)
            {
                // перенаправление ввода: только у первой команды
                if (i == 0 && infile) 
                {
                    int fd = open(infile, O_RDONLY); // открываем файл на чтение
                    if (fd == -1) { perror(infile); _exit(1); } // Если файла нет или нет прав на чтение
                    close(0); // Закрываем стандартный ввод: отключаем клавиатуру
                    dup(fd); // делаем копию дескриптора
                    close(fd); // Закрываем лишнюю копию, чтобы не оставлять в процессе открытый мусор.
                }
                if (i == ncmds - 1 && (outfile || appfile)) // только для последней команды строки
                {
                    // Открываем файл по-разному для > и >>
                    // > перезаписывает
                    // >> дописывает
                    int fd;
                    if (outfile)
                        fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    else
                        fd = open(appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);

                    if (fd == -1) { perror(outfile ? outfile : appfile); _exit(1); } // Если открыть не удалось, печатаем ошибку и завершаем копию.
                    close(1); // Закрываем стандартный вывод, то есть отключаем экран
                    dup(fd); // Кладем копию дескриптора в самый малый свободный номер
                    close(fd); // Убираем лишний номер
                }

                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs); // запускаем команду
                // execvp(имя, аргументы) ищет программу по имени в стандартных папках
                perror(cmds[i].cmdargs[0]); // сюда попадаем в случае, если команда не найдена
                _exit(1); // завершаем копию
            }
            
            if (bkgrnd) printf("[%d]\n", pid); // печатаем номер фонового процесса, если он был
            else waitpid(pid, NULL, 0); // иначе ждем окончания команды

        }

    }  /* close while */
}

/* PLACE SIGNAL CODE HERE */