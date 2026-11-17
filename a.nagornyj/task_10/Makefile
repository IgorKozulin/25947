CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2
TARGET = run_command
SRC = run_command.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
