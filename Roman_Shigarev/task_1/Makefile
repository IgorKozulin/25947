CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

all: options

options: options.c
	$(CC) $(CFLAGS) -o options options.c

clean:
	rm -f options

.PHONY: all clean
