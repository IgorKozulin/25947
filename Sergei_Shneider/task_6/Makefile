CC = gcc
CFLAGS = -O2
TARGET = line_index_alarm
SRC = line_index_alarm.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
