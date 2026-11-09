CC = gcc
CFLAGS = -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
TARGET = beeper
SRC = beeper.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
