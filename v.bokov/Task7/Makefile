CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = line2
SRC = line.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean