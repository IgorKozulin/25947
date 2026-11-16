
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

struct termios original_term;  // настройки терминала

void disable_raw_mode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term);
}

void enable_raw_mode(){
    tcgetattr(STDIN_FILENO, &original_term);

    atexit(disable_raw_mode);

    struct termios raw = original_term;

    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);

    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void redraw(const char *buffer, int pos, int *lines){
    printf("\r");

    for (int i = 0; i < *lines; i++){
        printf("\033[1A\r");
    }

    printf("\033[J");

    int col = 0;
    int i = 0;
    int current_lines = 0;

    while (i < pos){
        if (buffer[i] == ' '){
            if (col == 40){
                putchar('\n');
                current_lines++;
                col = 0;
            }

            putchar(' ');
            col++;
            i++;
        }
        else{
            int start = i;

            while (i < pos && buffer[i] != ' '){
                i++;
            }

            int length = i - start;

            if (col > 0 && col + length > 40){
                putchar('\n');
                current_lines++;
                col = 0;
            }

            for (int j = start; j < i; j++){
                if (col == 40){
                    putchar('\n');
                    current_lines++;
                    col = 0;
                }

                putchar(buffer[j]);
                col++;
            }
        }
    }

    *lines = current_lines;
    fflush(stdout);
}

int main(){
    enable_raw_mode();

    char buffer[4096];
    int pos = 0;
    int lines = 0;
    char c;

    char erase = original_term.c_cc[VERASE];
    char kill = original_term.c_cc[VKILL];

    while (1){
        if (read(STDIN_FILENO, &c, 1) != 1){
            break;
        }

        if (c == 4){
            if (pos == 0){
                printf("\nExiting...\n");
                break;
            }
            else{
                putchar('\a');
                fflush(stdout);
            }
        }
        else if (c == '\n' || c == '\r'){
            putchar('\n');
            pos = 0;
            lines = 0;
            fflush(stdout);
        }
        else if (c == erase){
            if (pos > 0){
                pos--;
                redraw(buffer, pos, &lines);
            }
        }
        else if (c == kill){
            pos = 0;
            redraw(buffer, pos, &lines);
        }
        else if (c == 23){
            while (pos > 0 && buffer[pos - 1] == ' '){
                pos--;
            }

            while (pos > 0 && buffer[pos - 1] != ' '){
                pos--;
            }

            while (pos > 0 && buffer[pos - 1] == ' '){
                pos--;
            }

            redraw(buffer, pos, &lines);
        }
        else if (isprint((unsigned char)c)){
            if (pos < 4095){
                buffer[pos] = c;
                pos++;

                redraw(buffer, pos, &lines);
            }
            else{
                putchar('\a');
                fflush(stdout);
            }
        }
        else{
            putchar('\a');
            fflush(stdout);
        }
    }

    return 0;
}

