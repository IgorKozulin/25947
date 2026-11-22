CC = gcc
CFLAGS = -O2 -std=gnu99
TARGET = pipe_uppe
SRC = pipe_upper.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
