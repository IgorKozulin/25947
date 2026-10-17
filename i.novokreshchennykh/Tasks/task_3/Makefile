CC = gcc
CFLAGS = -std=c99
TARGET = setuid
SOURCE = setuid.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
