CC = gcc
CFLAGS = -O2 -std=c99 -DDEBUG

all: myshell

myshell: shell.c promptline.c parseline.c shell.h
	$(CC) $(CFLAGS) -o myshell shell.c promptline.c parseline.c

clean:
	rm -f myshell *.o

.PHONY: all clean
