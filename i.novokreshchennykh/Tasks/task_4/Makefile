CC = gcc
CFLAGS = -std=c99
TARGET = list_strings
SOURCE = list_strings.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)
clean:
	rm -f $(TARGET)

.PHONY: all clean
