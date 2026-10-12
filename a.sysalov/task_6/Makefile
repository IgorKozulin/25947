CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = line_table_alarm
SRC = line_table_alarm.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
