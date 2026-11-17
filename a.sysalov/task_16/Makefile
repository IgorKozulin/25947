CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = single_answer
SRC = single_answer.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
