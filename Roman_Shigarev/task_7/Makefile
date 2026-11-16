CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

all: line_index_mmap

line_index_mmap: line_index_mmap.c
	$(CC) $(CFLAGS) -o line_index_mmap line_index_mmap.c

clean:
	rm -f line_index_mmap

.PHONY: all clean
