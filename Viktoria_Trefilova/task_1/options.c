#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *options = "ispuU:cC:dvV:";
    int c;

    while ((c = getopt(argc, argv, options)) != -1) {
        switch (c) {
            case 'i': printf("Option -i\n"); break;
            case 's': printf("Option -s\n"); break;
            case 'p': printf("Option -p\n"); break;
            case 'u': printf("Option -u\n"); break;
            case 'U': printf("Option -U, value: %s\n", optarg); break;
            case 'c': printf("Option -c\n"); break;
            case 'C': printf("Option -C, value: %s\n", optarg); break;
            case 'd': printf("Option -d\n"); break;
            case 'v': printf("Option -v\n"); break;
            case 'V': printf("Option -V, value: %s\n", optarg); break;
            case '?': printf("Invalid option: %c\n", optopt); break;
        }
    }
    return 0;
}
