#include <stdio.h>
#include "line_table.h"
#include "query.h"

int main(int argc, char **argv)
{
    struct LineTable table = {.fd = -1};
    int status = 0;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s TEXT_FILE\n", argv[0]);
        return 1;
    }
    if (open_line_table(&table, argv[1]) == -1) {
        perror(argv[1]);
        close_line_table(&table);
        return 1;
    }
    for (;;) {
        long number = 0;
        int result = query_line_number(0, &number);
        if (result == 1 || (result == 0 && number == 0))
            break;
        if (result == 3) {
            fprintf(stderr, "Invalid line number\n");
            continue;
        }
        if (result == -1) {
            perror("input");
            status = 1;
            break;
        }
        if ((unsigned long)number > table.count) {
            fprintf(stderr, "No such line\n");
            continue;
        }
        if (show_line(&table, (size_t)number - 1) == -1) {
            perror("output");
            status = 1;
            break;
        }
    }
    close_line_table(&table);
    return status;
}
