CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDLIBS =
# On Solaris: make LDLIBS="-lsocket -lnsl"
TARGETS = server client

all: $(TARGETS)

server: server.c socket_io.h
	$(CC) $(CFLAGS) -o server server.c $(LDLIBS)

client: client.c socket_io.h
	$(CC) $(CFLAGS) -o client client.c $(LDLIBS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
