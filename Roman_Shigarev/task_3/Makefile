CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

all: setuid_test

setuid_test: setuid_test.c
	$(CC) $(CFLAGS) -o setuid_test setuid_test.c

clean:
	rm -f setuid_test

.PHONY: all clean
