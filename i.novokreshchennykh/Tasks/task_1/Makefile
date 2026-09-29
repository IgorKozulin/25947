CC = gcc
CFLAGS = -O2
TARGET = options
SOURCE = options.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

.PHONY: all clear

clean:
	rm -f $(TARGET)
