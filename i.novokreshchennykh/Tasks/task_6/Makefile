CC = gcc
CFLAGS = -std=c99
TARGET = line
SOURCE = line.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) 

clean:
	rm -f $(TARGET)

.PHONY: all clean
