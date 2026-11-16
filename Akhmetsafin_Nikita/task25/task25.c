
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int send_text(int fd)
{
    FILE *output = fdopen(fd, "w");
    if (output == NULL) {
        perror("fdopen");
        close(fd);
        return EXIT_FAILURE;
    }

    setvbuf(output, NULL, _IOLBF, 0);
    int result = EXIT_SUCCESS;
    int ch;
    while ((ch = getchar()) != EOF) {
        if (putc(ch, output) == EOF) {
            perror("pipe write");
            result = EXIT_FAILURE;
            break;
        }
    }

    if (ferror(stdin)) {
        perror("stdin");
        result = EXIT_FAILURE;
    }
    if (fclose(output) == EOF) {
        perror("pipe close");
        result = EXIT_FAILURE;
    }
    return result;
}

static int print_uppercase(int fd)
{
    FILE *input = fdopen(fd, "r");
    if (input == NULL) {
        perror("fdopen");
        close(fd);
        return EXIT_FAILURE;
    }

    int result = EXIT_SUCCESS;
    int ch;
    while ((ch = getc(input)) != EOF) {
        if (putchar(toupper(ch)) == EOF) {
            result = EXIT_FAILURE;
            break;
        }
    }

    if (result != EXIT_SUCCESS || fflush(stdout) == EOF) {
        perror("stdout");
        result = EXIT_FAILURE;
    }
    if (ferror(input)) {
        perror("pipe read");
        result = EXIT_FAILURE;
    }
    fclose(input);
    return result;
}

int main(void)
{
    int channel[2];
    if (pipe(channel) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        close(channel[0]);
        close(channel[1]);
        return EXIT_FAILURE;
    }

    if (child == 0) {
        close(channel[0]);
        _exit(send_text(channel[1]));
    }

    close(channel[1]);
    int result = print_uppercase(channel[0]);
    if (result != EXIT_SUCCESS) {
        kill(child, SIGTERM);
    }

    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return EXIT_FAILURE;
    }
    return result;
}
