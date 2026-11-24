CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = upper

all: $(TARGET)

$(TARGET): upper.c
	$(CC) $(CFLAGS) -o $(TARGET) upper.c

clean:
	rm -f $(TARGET)

.PHONY: all clean