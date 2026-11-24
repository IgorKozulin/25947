CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -D_POSIX_C_SOURCE=200112L
LDLIBS = -lsocket -lnsl -lrt

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDLIBS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDLIBS)

clean:
	rm -f server client

.PHONY: all clean
