CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = my_ls

all: $(TARGET)

$(TARGET): my_ls.c
	$(CC) $(CFLAGS) -o $(TARGET) my_ls.c

clean:
	rm -f $(TARGET)

.PHONY: all clean