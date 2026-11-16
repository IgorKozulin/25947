CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = list_strings
SRC = list.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean