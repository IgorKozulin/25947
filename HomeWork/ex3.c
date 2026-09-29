#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

void print_ids()
{
    printf("реальный UID %d\n", getuid());
    printf("эффективный UID %d\n", geteuid());
}

void try_open(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror(path);
    }
    else
    {
        printf("файл %s успешно открыт\n", path);
        fclose(fp);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) 
    {
        printf("usage: %s file\n", argv[0]);
        return 1;
    }
    print_ids();
    try_open(argv[1]);
    
    setuid(getuid());
    print_ids();
    try_open(argv[1]);
    return 0;
}
