CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2
TARGET = beeper
SRC = beeper.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
