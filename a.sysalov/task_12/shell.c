#define _POSIX_C_SOURCE 200809L
#define STAGE 12
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

struct Command {
    char *arguments[128];
    char *input;
    char *output;
    int append;
    int background;
};

int tokenize(const char *line, char *storage, char *tokens[], int types[])
{
    int count = 0;
    while (*line != '\0') {
        if (isspace((unsigned char)*line)) {
            line++;
            continue;
        }
        if (count == 255) {
            return -1;
        }
        types[count] = 0;
        tokens[count++] = storage;
        if (strchr(";&<>|", *line) != NULL) {
            char symbol = *line++;
            types[count - 1] = symbol;
            *storage++ = symbol;
            if (symbol == '>' && *line == '>') {
                *storage++ = *line++;
            }
        } else {
            while (*line && !isspace((unsigned char)*line)
                    && strchr(";&<>|", *line) == NULL) {
                if (*line == '\'' || *line == '"') {
                    char quote = *line++;
                    while (*line && *line != quote) {
                        *storage++ = *line++;
                    }
                    if (*line == '\0') {
                        return -1;
                    }
                    line++;
                } else if (*line == '\\' && line[1]) {
                    line++;
                    *storage++ = *line++;
                } else {
                    *storage++ = *line++;
                }
            }
        }
        *storage++ = '\0';
    }
    tokens[count] = NULL;
    return count;
}

int run_command(struct Command *command)
{
    if (STAGE == 12) {
        printf("Command:");
        for (int i = 0; command->arguments[i] != NULL; i++) {
            printf(" [%s]", command->arguments[i]);
        }
        printf("\n");
        if (command->input) printf("Input: %s\n", command->input);
        if (command->output) printf("Output: %s (%s)\n", command->output,
                                    command->append ? "append" : "write");
        if (command->background) printf("Background: yes\n");
        return 0;
    }
    if (strcmp(command->arguments[0], "exit") == 0) {
        return 1;
    }
    if (strcmp(command->arguments[0], "cd") == 0) {
        const char *directory = command->arguments[1];
        if (directory == NULL) directory = getenv("HOME");
        if (directory == NULL || chdir(directory) == -1) {
            fprintf(stderr, "Cannot change directory.\n");
        }
        return 0;
    }

    fflush(stdout);
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return 0;
    }
    if (child == 0) {
        if (command->input != NULL) {
            int fd = open(command->input, O_RDONLY);
            if (fd == -1 || dup2(fd, STDIN_FILENO) == -1) {
                perror(command->input);
                _exit(1);
            }
            if (fd != STDIN_FILENO) close(fd);
        }
        if (command->output != NULL) {
            int flags = O_WRONLY | O_CREAT;
            flags |= command->append ? O_APPEND : O_TRUNC;
            int fd = open(command->output, flags, 0666);
            if (fd == -1 || dup2(fd, STDOUT_FILENO) == -1) {
                perror(command->output);
                _exit(1);
            }
            if (fd != STDOUT_FILENO) close(fd);
        }
        execvp(command->arguments[0], command->arguments);
        perror(command->arguments[0]);
        _exit(127);
    }
    if (command->background) {
        printf("Background PID: %ld\n", (long)child);
    } else {
        while (waitpid(child, NULL, 0) == -1) {
            if (errno != EINTR) {
                perror("waitpid");
                break;
            }
        }
    }
    return 0;
}

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    int finish = 0;

    /* Keep stdio from reading input intended for a foreground child. */
    setvbuf(stdin, NULL, _IONBF, 0);
    while (!finish) {
        while (waitpid(-1, NULL, WNOHANG) > 0) {}
        if (isatty(STDIN_FILENO)) {
            printf("shell> ");
            fflush(stdout);
        }
        ssize_t length = getline(&line, &capacity, stdin);
        if (length == -1) break;
        char *storage = malloc(2 * (size_t)length + 1);
        if (storage == NULL) {
            perror("malloc");
            free(line);
            return 1;
        }
        char *tokens[256];
        int types[256];
        int count = tokenize(line, storage, tokens, types);
        int error = count < 0;
        struct Command commands[128] = {0};
        int command_count = 0;
        int i = 0;
        while (!error && i < count) {
            struct Command *command = &commands[command_count];
            int arguments = 0;
            while (i < count && types[i] != ';' && types[i] != '&') {
                int type = types[i];
                char *token = tokens[i++];
                if (type == '|') {
                    error = 1;
                    break;
                }
                if (type == '<' || type == '>') {
                    if ((STAGE > 12 && STAGE < 15) || i == count
                            || types[i] != 0) {
                        error = 1;
                        break;
                    }
                    if (token[0] == '<') {
                        if (command->input) error = 1;
                        command->input = tokens[i++];
                    } else {
                        if (command->output) error = 1;
                        command->output = tokens[i++];
                        command->append = token[1] == '>';
                    }
                } else if (arguments < 127) {
                    command->arguments[arguments++] = token;
                } else {
                    error = 1;
                    break;
                }
            }
            if (arguments == 0) error = 1;
            if (i < count && types[i] == '&') {
                if (STAGE == 13) error = 1;
                command->background = 1;
            }
            if (i < count) i++;
            command_count++;
        }
        if (error) {
            fprintf(stderr, "Invalid or unsupported command syntax.\n");
        } else {
            for (i = 0; i < command_count && !finish; i++) {
                finish = run_command(&commands[i]);
            }
        }
        free(storage);
    }
    free(line);
    return 0;
}
