#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void print_uid(){
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
}

void check_file(){
    FILE *file = fopen("data.txt", "r");

    if (file == NULL){
        perror("fopen");
    }else{
        printf("successfully\n");
        fclose(file);
    }
}

int main(){
    print_uid();
    check_file();

    if (setuid(getuid()) == -1){
        perror("setuid");
        return 1;
    }
    print_uid();
    check_file();
    return 0;
}
