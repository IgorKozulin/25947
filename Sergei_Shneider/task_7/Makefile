CC = gcc
CFLAGS = -O2
TARGET = line_index_mmap
SRC = line_index_mmap.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
