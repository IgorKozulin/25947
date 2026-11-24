CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = line_editor

all: $(TARGET)

$(TARGET): line_editor.c
	$(CC) $(CFLAGS) -o $(TARGET) line_editor.c

clean:
	rm -f $(TARGET)

.PHONY: all clean