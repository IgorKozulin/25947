CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = file_pattern
SRC = file_pattern.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
