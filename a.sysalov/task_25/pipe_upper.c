#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int copy_text(int input, int output, int uppercase)
{
    unsigned char buffer[1024];

    while (1) {
        ssize_t length = read(input, buffer, sizeof(buffer));
        if (length == -1) {
            if (errno == EINTR) continue;
            perror("read");
            return 1;
        }
        if (length == 0) return 0;

        if (uppercase) {
            for (ssize_t i = 0; i < length; i++) {
                buffer[i] = (unsigned char)toupper(buffer[i]);
            }
        }
        ssize_t total = 0;
        while (total < length) {
            ssize_t written = write(output, buffer + total, (size_t)(length - total));
            if (written == -1 && errno == EINTR) continue;
            if (written <= 0) {
                if (written == 0) errno = EIO;
                perror("write");
                return 1;
            }
            total += written;
        }
    }
}

int main(void)
{
    struct sigaction action = {0};
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGPIPE, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    int channel[2];
    if (pipe(channel) == -1) {
        perror("pipe");
        return 1;
    }
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        close(channel[0]);
        close(channel[1]);
        return 1;
    }
    if (child == 0) {
        close(channel[1]);
        int status = copy_text(channel[0], STDOUT_FILENO, 1);
        close(channel[0]);
        _exit(status);
    }

    close(channel[0]);
    int status = copy_text(STDIN_FILENO, channel[1], 0);
    close(channel[1]);

    int child_status;
    while (waitpid(child, &child_status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }
    return status || !WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0;
}
