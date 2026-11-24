CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = beeper

all: $(TARGET)

$(TARGET): beeper.c
	$(CC) $(CFLAGS) -o $(TARGET) beeper.c

clean:
	rm -f $(TARGET)

.PHONY: all clean