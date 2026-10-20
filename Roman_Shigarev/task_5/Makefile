CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

all: line_index

line_index: line_index.c
	$(CC) $(CFLAGS) -o line_index line_index.c

clean:
	rm -f line_index

.PHONY: all clean
