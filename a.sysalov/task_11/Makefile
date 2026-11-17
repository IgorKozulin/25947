CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = execvpe_test
SRC = execvpe_test.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
