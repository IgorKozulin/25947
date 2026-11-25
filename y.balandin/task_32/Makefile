CC = gcc
CFLAGS = -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200112L
LDFLAGS = -lsocket -lnsl -lrt
TARGETS = server client

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o socket

.PHONY: all clean
