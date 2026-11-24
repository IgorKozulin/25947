#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <signal.h>

#define MAX_TEXT 4096
#define WIDTH 40

struct termios old_term;
char buffer[MAX_TEXT];
int pos = 0;
int screen_lines = 1;

void restore_terminal(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
}

void stop_program(int sig)
{
    (void)sig;
    restore_terminal();
    _exit(1);
}

void redraw(void)
{
    int col = 0;
    int rows = 1;

    printf("\r");

    for (int i = 1; i < screen_lines; i++)
        printf("\033[A\r");

    for (int i = 0; i < screen_lines; i++)
    {
        printf("\033[2K");

        if (i < screen_lines - 1)
            printf("\033[B\r");
    }

    for (int i = 1; i < screen_lines; i++)
        printf("\033[A\r");

    int i = 0;

    while (i < pos)
    {
        if (buffer[i] == ' ')
        {
            if (col == WIDTH)
            {
                printf("\n");
                col = 0;
                rows++;
            }

            putchar(' ');
            col++;
            i++;
        }
        else
        {
            int start = i;

            while (i < pos && buffer[i] != ' ')
                i++;

            int len = i - start;

            if (col > 0 && col + len > WIDTH)
            {
                printf("\n");
                col = 0;
                rows++;
            }

            for (int j = start; j < i; j++)
            {
                if (col == WIDTH)
                {
                    printf("\n");
                    col = 0;
                    rows++;
                }

                putchar(buffer[j]);
                col++;
            }
        }
    }

    screen_lines = rows;
    fflush(stdout);
}

int main(void)
{
    if (tcgetattr(STDIN_FILENO, &old_term) == -1)
    {
        perror("tcgetattr");
        return 1;
    }

    struct termios new_term = old_term;

    new_term.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_term) == -1)
    {
        perror("tcsetattr");
        return 1;
    }

    atexit(restore_terminal);
    signal(SIGINT, stop_program);
    signal(SIGTERM, stop_program);

    unsigned char erase = old_term.c_cc[VERASE];
    unsigned char kill = old_term.c_cc[VKILL];

    unsigned char c;

    printf("Line editor (40 columns)\n");
    fflush(stdout);

    while (read(STDIN_FILENO, &c, 1) == 1)
    {
        if (c == 4)
        {
            if (pos == 0)
            {
                printf("\r\nExiting...\n");
                break;
            }

            putchar('\a');
            fflush(stdout);
        }
        else if (c == erase || c == 8 || c == 127)
        {
            if (pos > 0)
            {
                pos--;
                buffer[pos] = '\0';
                redraw();
            }
            else
            {
                putchar('\a');
                fflush(stdout);
            }
        }
        else if (c == kill)
        {
            pos = 0;
            buffer[0] = '\0';
            redraw();
        }
        else if (c == 23)
        {
            while (pos > 0 && buffer[pos - 1] == ' ')
                pos--;

            while (pos > 0 && buffer[pos - 1] != ' ')
                pos--;

            buffer[pos] = '\0';
            redraw();
        }
        else if (c == '\n' || c == '\r')
        {
            printf("\r\n");
            pos = 0;
            buffer[0] = '\0';
            screen_lines = 1;
            fflush(stdout);
        }
        else if (isprint(c))
        {
            if (pos < MAX_TEXT - 1)
            {
                buffer[pos++] = c;
                buffer[pos] = '\0';
                redraw();
            }
            else
            {
                putchar('\a');
                fflush(stdout);
            }
        }
        else
        {
            putchar('\a');
            fflush(stdout);
        }
    }

    return 0;
}