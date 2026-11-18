CC = gcc
CFLAGS = -O2 -Wall -std=gnu99
TARGET = pipe_upper
SRC = pipe_upper.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
