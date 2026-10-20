CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

all: line_index_alarm

line_index_alarm: line_index_alarm.c
	$(CC) $(CFLAGS) -o line_index_alarm line_index_alarm.c

clean:
	rm -f line_index_alarm

.PHONY: all clean
