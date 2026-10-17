CC = gcc
CFLAGS = -std=c99
TARGET = line_lookup
SOURCE = line_lookup.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
