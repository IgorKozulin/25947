#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define WIDTH 40

static struct termios saved_terminal;
static int terminal_changed;
static volatile sig_atomic_t interrupted;

static void restore_terminal(void)
{
    if (terminal_changed) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_terminal);
        terminal_changed = 0;
    }
}

static void on_signal(int number)
{
    interrupted = number;
}

static int print_bytes(const char *text, size_t length)
{
    while (length > 0) {
        ssize_t written = write(STDOUT_FILENO, text, length);

        if (written < 0) {
            if (errno == EINTR && !interrupted) {
                continue;
            }

            return -1;
        }

        if (written == 0) {
            return -1;
        }

        text += written;
        length -= (size_t)written;
    }

    return 0;
}

static int erase_last(size_t *length)
{
    if (*length == 0) {
        return 0;
    }

    if (print_bytes("\b \b", 3) == -1) {
        return -1;
    }

    --*length;
    return 0;
}

int main(void)
{
    struct termios edited_terminal;
    struct sigaction action;
    char line[WIDTH];
    size_t length = 0;
    unsigned char key;
    int error = 0;

    if (tcgetattr(STDIN_FILENO, &saved_terminal) == -1) {
        perror("tcgetattr");
        return 1;
    }

    if (atexit(restore_terminal) != 0) {
        fprintf(stderr, "Cannot register terminal restoration\n");
        return 1;
    }

    memset(&action, 0, sizeof action);
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) == -1 ||
        sigaction(SIGTERM, &action, NULL) == -1 ||
        sigaction(SIGHUP, &action, NULL) == -1 ||
        sigaction(SIGQUIT, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    edited_terminal = saved_terminal;

    edited_terminal.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    edited_terminal.c_iflag &= ~(IXON | ICRNL | INLCR | IGNCR | ISTRIP);
    edited_terminal.c_oflag &= ~OPOST;

    edited_terminal.c_cc[VMIN] = 1;
    edited_terminal.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &edited_terminal) == -1) {
        perror("tcsetattr");
        return 1;
    }

    terminal_changed = 1;

    while (!interrupted) {
        ssize_t count = read(STDIN_FILENO, &key, 1);

        if (count == 0) {
            break;
        }

        if (count == -1) {
            if (errno == EINTR && !interrupted) {
                continue;
            }

            if (!interrupted) {
                perror("read");
                error = 1;
            }

            break;
        }

        if (key == 4) {
            if (length == 0) {
                break;
            }

            if (print_bytes("\a", 1) == -1) {
                error = 1;
                break;
            }
        } else if (key == '\r' || key == '\n') {
            if (print_bytes("\r\n", 2) == -1) {
                error = 1;
                break;
            }

            length = 0;
        } else if ((saved_terminal.c_cc[VERASE] != _POSIX_VDISABLE &&
                    key == saved_terminal.c_cc[VERASE]) ||
                   key == 127 || key == 8) {
            if (erase_last(&length) == -1) {
                error = 1;
                break;
            }
        } else if ((saved_terminal.c_cc[VKILL] != _POSIX_VDISABLE &&
                    key == saved_terminal.c_cc[VKILL]) ||
                   key == 21) {
            while (length > 0) {
                if (erase_last(&length) == -1) {
                    error = 1;
                    break;
                }
            }

            if (error) {
                break;
            }
        } else if (key == 23) {
            while (length > 0 && line[length - 1] == ' ') {
                if (erase_last(&length) == -1) {
                    error = 1;
                    break;
                }
            }

            while (!error && length > 0 && line[length - 1] != ' ') {
                if (erase_last(&length) == -1) {
                    error = 1;
                    break;
                }
            }

            if (error) {
                break;
            }
        } else if (!isprint(key)) {
            if (print_bytes("\a", 1) == -1) {
                error = 1;
                break;
            }
        } else {
            if (length == WIDTH) {
                size_t start = length;
                char word[WIDTH];
                size_t word_length;

                if (key != ' ') {
                    while (start > 0 && line[start - 1] != ' ') {
                        --start;
                    }
                }

                if (start == 0) {
                    start = length;
                }

                word_length = length - start;
                memcpy(word, line + start, word_length);

                while (length > start) {
                    if (erase_last(&length) == -1) {
                        error = 1;
                        break;
                    }
                }

                if (error || print_bytes("\r\n", 2) == -1) {
                    error = 1;
                    break;
                }

                memcpy(line, word, word_length);
                length = word_length;

                if (print_bytes(word, word_length) == -1) {
                    error = 1;
                    break;
                }
            }

            line[length++] = (char)key;

            if (print_bytes((const char *)&key, 1) == -1) {
                error = 1;
                break;
            }
        }
    }

    restore_terminal();

    if (interrupted) {
        int number = interrupted;
        signal(number, SIG_DFL);
        raise(number);
        return 128 + number;
    }

    return error ? 1 : 0;
}
