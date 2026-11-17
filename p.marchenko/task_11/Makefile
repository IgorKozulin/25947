CC = gcc
CFLAGS = -O2 -std=gnu99
TARGET = my_execvpe
SRC = my_execvpe.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
