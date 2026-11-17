#define _POSIX_C_SOURCE 199506L

#include <signal.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main(void)
{
    sigset_t set;
    struct termios saved, term;
    int sig, count = 0, result = 1;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1 ||
        tcgetattr(STDIN_FILENO, &saved) == -1)
        return 1;

    term = saved;
    term.c_lflag &= ~ECHO;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1)
        return 1;
// 
    while (sigwait(&set, &sig) == 0) {
        if (sig == SIGQUIT) {
            printf("\nBeeps: %d\n", count);
            result = 0;
            break;
        }

        if (count < 3) {
            if (write(STDOUT_FILENO, "\a", 1) != 1)
                break;
            count++;
        }
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &saved) == -1)
        return 1;

    return result;
}
