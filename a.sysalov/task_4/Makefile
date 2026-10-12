CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = string_list
SRC = string_list.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
