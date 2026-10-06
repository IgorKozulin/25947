#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

char *file;
long file_size;

struct LineInfo{
    long offset;
    int length;
};

void alarm_handler(int sig)
{
    fwrite(file, 1, file_size, stdout);
    fflush(stdout);

    munmap(file, file_size);
    _exit(0);
}

int main(int args, char **argv){
    int fd = open(argv[1], O_RDONLY);

    struct stat st;
    fstat(fd, &st);

    file_size = st.st_size;
    file = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);

    close(fd);

    struct LineInfo table[100];
    int lines = 0;
    long start = 0;

    for (long i = 0; i < file_size; i++){

        if (file[i] == '\n'){

            table[lines].offset = start;
            table[lines].length = i - start + 1;

            lines++;
            start = i + 1;
        }
    }

    if (start < file_size){
        table[lines].offset = start;
        table[lines].length = file_size - start;
        lines++;
    }

    for (int i = 0; i < lines; i++){
        printf("Line %d: offset = %ld, length = %d\n",
            i + 1, table[i].offset, table[i].length);
    }

    signal(SIGALRM, alarm_handler);

    int number;

    while (1){
        printf("Enter line number (0 to quit): ");
        fflush(stdout);

        alarm(5);

        if (scanf("%d", &number) != 1){
            alarm(0);
            break;
        }

        alarm(0);

        if (number == 0){
            break;
        }

        int index = number - 1;

        fwrite(file + table[index].offset,
               1,
               table[index].length,
               stdout);
    }

    munmap(file, file_size);

    return 0;
}
