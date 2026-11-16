CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra
TARGET = ex_time
SRC = ex_time.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
