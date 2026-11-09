#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t beep_count = 0;
volatile sig_atomic_t finished = 0;

void sigint_handler(int sig){
    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

void sigquit_handler(int sig){
    finished = 1;
}

int main(){
    struct sigaction sa_int;
    struct sigaction sa_quit;

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;
    sigaction(SIGQUIT, &sa_quit, NULL);

    printf("Beeper started\n");
    printf("Press Ctrl+C to beep\n");
    printf("Press Ctrl+\\ to quit\n");

    while (!finished){
        pause();
    }

    printf("\nTotal beeps: %d\n", (int)beep_count);
    printf("Program finished\n");

    return 0;
}
