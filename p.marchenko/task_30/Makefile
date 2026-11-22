CC = gcc
CFLAGS = -O2 -std=gnu99

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c -lsocket -lnsl

client: client.c
	$(CC) $(CFLAGS) -o client client.c -lsocket -lnsl

clean:
	rm -f server client

.PHONY: all clean
