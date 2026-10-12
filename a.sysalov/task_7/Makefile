CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = line_table_mmap
SRC = line_table_mmap.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
