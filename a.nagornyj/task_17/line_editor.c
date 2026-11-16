#define _POSIX_C_SOURCE 200809L
#define __EXTENSIONS__ 1

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define INPUT_CAPACITY 4096
#define DISPLAY_WIDTH 40

static struct termios saved_terminal;
static int terminal_saved;
static volatile sig_atomic_t interrupted_signal;

static void
restore_terminal(void)
{
    if (terminal_saved) {
        (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_terminal);
        terminal_saved = 0;
    }
}

static void
remember_signal(int signal_number)
{
    interrupted_signal = signal_number;
}

static void
beep(void)
{
    (void)putchar('\a');
    (void)fflush(stdout);
}

static int
enable_character_input(void)
{
    struct termios raw_terminal;

    if (tcgetattr(STDIN_FILENO, &saved_terminal) == -1) {
        return -1;
    }
    terminal_saved = 1;
    if (atexit(restore_terminal) != 0) {
        errno = ENOMEM;
        return -1;
    }

    raw_terminal = saved_terminal;
    raw_terminal.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
    raw_terminal.c_iflag &= (tcflag_t)~(IXON | IXOFF);
    raw_terminal.c_cc[VMIN] = 1;
    raw_terminal.c_cc[VTIME] = 0;
    return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_terminal);
}

static int
install_interrupt_handlers(void)
{
    struct sigaction action;
    const int signals[] = { SIGINT, SIGTERM, SIGHUP };

    memset(&action, 0, sizeof(action));
    action.sa_handler = remember_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    for (size_t index = 0; index < sizeof(signals) / sizeof(signals[0]);
            ++index) {
        if (sigaction(signals[index], &action, NULL) == -1) {
            return -1;
        }
    }
    return 0;
}

static int
redraw(const char *buffer, size_t length, size_t *previous_rows)
{
    struct winsize window;
    size_t wrap_width = DISPLAY_WIDTH;
    size_t column = 2;
    size_t rows = 1;
    size_t position = 0;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == 0 &&
        window.ws_col > 0 && window.ws_col < wrap_width) {
        wrap_width = window.ws_col;
    }
    if (*previous_rows > 1 &&
        printf("\033[%zuA", *previous_rows - 1) < 0) {
        return -1;
    }
    if (printf("\r\033[J> ") < 0) {
        return -1;
    }

    while (position < length) {
        if (isspace((unsigned char)buffer[position])) {
            if (column == wrap_width) {
                if (putchar('\n') == EOF) {
                    return -1;
                }
                column = 0;
                ++rows;
            }
            if (putchar((unsigned char)buffer[position]) == EOF) {
                return -1;
            }
            ++column;
            ++position;
        } else {
            size_t word_end = position;
            size_t word_length;

            while (word_end < length &&
                    !isspace((unsigned char)buffer[word_end])) {
                ++word_end;
            }
            word_length = word_end - position;
            if (column > 0 && word_length <= wrap_width &&
                column + word_length > wrap_width) {
                if (putchar('\n') == EOF) {
                    return -1;
                }
                column = 0;
                ++rows;
            }
            while (position < word_end) {
                if (column == wrap_width) {
                    if (putchar('\n') == EOF) {
                        return -1;
                    }
                    column = 0;
                    ++rows;
                }
                if (putchar((unsigned char)buffer[position]) == EOF) {
                    return -1;
                }
                ++column;
                ++position;
            }
        }
    }

    *previous_rows = rows;
    return fflush(stdout) == EOF ? -1 : 0;
}

int
main(void)
{
    char buffer[INPUT_CAPACITY];
    size_t length = 0;
    size_t rendered_rows = 1;
    unsigned char character;
    ssize_t amount;

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "This line editor requires a terminal.\n");
        return EXIT_FAILURE;
    }
    if (enable_character_input() == -1) {
        perror("enabling character input");
        return EXIT_FAILURE;
    }
    if (install_interrupt_handlers() == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("> ");
    if (fflush(stdout) == EOF) {
        perror("fflush");
        return EXIT_FAILURE;
    }

    for (;;) {
        if (interrupted_signal != 0) {
            putchar('\n');
            return EXIT_SUCCESS;
        }
        amount = read(STDIN_FILENO, &character, 1);
        if (amount == -1 && errno == EINTR && interrupted_signal != 0) {
            putchar('\n');
            return EXIT_SUCCESS;
        }
        if (amount == -1 && errno == EINTR) {
            continue;
        }
        if (amount == -1) {
            perror("read");
            return EXIT_FAILURE;
        }
        if (amount == 0) {
            putchar('\n');
            return EXIT_SUCCESS;
        }

        if (character == 4) {
            if (length == 0) {
                putchar('\n');
                return EXIT_SUCCESS;
            }
            beep();
        } else if (character == saved_terminal.c_cc[VERASE]) {
            if (length > 0) {
                --length;
                if (redraw(buffer, length, &rendered_rows) == -1) {
                    perror("redraw");
                    return EXIT_FAILURE;
                }
            } else {
                beep();
            }
        } else if (character == saved_terminal.c_cc[VKILL]) {
            length = 0;
            if (redraw(buffer, length, &rendered_rows) == -1) {
                perror("redraw");
                return EXIT_FAILURE;
            }
        } else if (character == 23) {
            while (length > 0 && isspace((unsigned char)buffer[length - 1])) {
                --length;
            }
            while (length > 0 &&
                    !isspace((unsigned char)buffer[length - 1])) {
                --length;
            }
            while (length > 0 && isspace((unsigned char)buffer[length - 1])) {
                --length;
            }
            if (redraw(buffer, length, &rendered_rows) == -1) {
                perror("redraw");
                return EXIT_FAILURE;
            }
        } else if (character == '\n' || character == '\r') {
            if (printf("\r\n> ") < 0 || fflush(stdout) == EOF) {
                perror("writing prompt");
                return EXIT_FAILURE;
            }
            length = 0;
            rendered_rows = 1;
        } else if (isprint(character)) {
            if (length + 1 < sizeof(buffer)) {
                buffer[length++] = (char)character;
                if (redraw(buffer, length, &rendered_rows) == -1) {
                    perror("redraw");
                    return EXIT_FAILURE;
                }
            } else {
                beep();
            }
        } else {
            beep();
        }
    }
}
