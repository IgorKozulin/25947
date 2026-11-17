CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = locked_editor
SRC = locked_editor.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
