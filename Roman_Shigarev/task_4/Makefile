CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

all: list_strings

list_strings: list_strings.c
	$(CC) $(CFLAGS) -o list_strings list_strings.c

clean:
	rm -f list_strings

.PHONY: all clean
