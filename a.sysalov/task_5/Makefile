CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = line_table
SRC = line_table.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
