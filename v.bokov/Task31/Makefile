CC = gcc
CFLAGS = -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200112L
LIBS = -lsocket -lnsl

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LIBS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LIBS)

clean:
	rm -f server client socket

.PHONY: all clean