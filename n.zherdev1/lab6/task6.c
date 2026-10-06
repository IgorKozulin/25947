#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINES 1000
#define MAX_LENGTH 1024

int fd;

void timeout(int signal)
{
    char buffer[1024];
    int n;

    lseek(fd, 0, SEEK_SET);

    while ((n = read(fd, buffer, sizeof(buffer))) > 0)
        write(STDOUT_FILENO, buffer, n);

    close(fd);
    _exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        return 1;

    fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    off_t offsets[MAX_LINES];
    int lengths[MAX_LINES];
    int lines = 0;

    offsets[0] = 0;

    char c;

    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            off_t pos = lseek(fd, 0, SEEK_CUR);

            lengths[lines] = pos - offsets[lines];
            lines++;

            if (lines < MAX_LINES)
                offsets[lines] = pos;
        }
    }

    off_t end = lseek(fd, 0, SEEK_CUR);

    if (lines < MAX_LINES && end > offsets[lines]) {
        lengths[lines] = end - offsets[lines];
        lines++;
    }

    signal(SIGALRM, timeout);

    int number;
    char buffer[MAX_LENGTH + 1];

    while (1) {
        printf("Line number: ");
        fflush(stdout);

        alarm(5);

        if (scanf("%d", &number) != 1)
            break;

        alarm(0);

        if (number == 0)
            break;

        if (number < 1 || number > lines) {
            printf("No such line\n");
            continue;
        }

        int i = number - 1;

        lseek(fd, offsets[i], SEEK_SET);

        int n = read(fd, buffer, lengths[i]);
        buffer[n] = '\0';

        printf("%s", buffer);

        if (n > 0 && buffer[n - 1] != '\n')
            printf("\n");
    }

    close(fd);

    return 0;
}
