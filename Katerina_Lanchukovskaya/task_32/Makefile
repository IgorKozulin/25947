CC = gcc
CFLAGS = -O2 -std=c99 -D_POSIX_C_SOURCE=200112L
LDFLAGS = -lsocket -lnsl -lrt

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

clean:
	rm -f server client *.o socket

.PHONY: all clean
