#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define MAX 10000

off_t start[MAX], length[MAX];
int count = 0;

void fail(const char *s)
{
    perror(s);
    exit(1);
}

void add(off_t begin, off_t end)
{
    if (count == MAX) { fputs("Too many lines\n", stderr); exit(1); }
    start[count] = begin;
    length[count++] = end - begin;
}

volatile sig_atomic_t expired = 0;
sigset_t mask;

void handler(int sig)
{
    (void)sig;
    expired = 1;
}

int input(void)
{
    fd_set set;
    int result;
    do {
        FD_ZERO(&set);
        FD_SET(0, &set);
        result = pselect(1, &set, NULL, NULL, NULL, &mask);
        if (expired) return EOF;
    } while (result < 0 && errno == EINTR);
    if (result < 0) fail("pselect");
    return getchar();
}

char *data;
void show(int fd, off_t begin, off_t size)
{
    (void)fd;
    if (size && fwrite(data + begin, 1, size, stdout) != (size_t)size)
        fail("stdout");
}

int main(int argc, char **argv)
{
    int fd, number, c, bad, digits;
    off_t total = 0, begin = 0;
    struct stat st;
    off_t i;
    if (argc != 2) { fprintf(stderr, "Usage: %s file\n", argv[0]); return 1; }
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) fail("open");
    if (fstat(fd, &st) == -1) fail("fstat");
    total = st.st_size;
    if (!S_ISREG(st.st_mode) || total < 0 || (off_t)(size_t)total != total)
        return 1;
    if (total) {
        data = mmap(NULL, (size_t)total, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) fail("mmap");
    }
    for (i = 0; i < total; i++) {
        if (data[i] == '\n') { add(begin, i + 1); begin = i + 1; }
    }
    if (begin < total) add(begin, total);
    for (int i = 0; i < count; i++) {
        printf("Line %d: start=%ld,   length=%ld\n",
               i + 1, (long)start[i], (long)length[i]);
    }
    printf("\n");
    struct sigaction sa = {0};
    sigset_t blocked;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) == -1) fail("sigaction");
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &blocked, &mask) == -1) fail("sigprocmask");
    sigdelset(&mask, SIGALRM);
    setbuf(stdin, NULL);
    for (;;) {
        printf("Line (0 = exit): ");
        fflush(stdout);
        number = bad = digits = 0;
        alarm(5);
        while ((c = input()) != '\n' && c != EOF) {
            if (c < '0' || c > '9' || number > MAX) bad = 1;
            else { number = number * 10 + c - '0'; digits = 1; }
        }
        alarm(0);
        if (expired) { puts("\nTime is up:"); show(fd, 0, total); break; }
        if (ferror(stdin)) fail("stdin");
        if (c == EOF && !digits) break;
        if (bad || !digits || number > count) { puts("Invalid number"); continue; }
        if (!number) break;
        show(fd, start[number - 1], length[number - 1]);
        putchar('\n');
    }
    if (total && munmap(data, (size_t)total) == -1) fail("munmap");
    if (close(fd) == -1) fail("close");
    return 0;
}

