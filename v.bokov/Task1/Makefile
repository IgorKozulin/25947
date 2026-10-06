CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
TARGET = options
SRC = options.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
.PHONY: all clean
