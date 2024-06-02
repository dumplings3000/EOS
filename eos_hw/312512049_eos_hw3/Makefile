CC = gcc

CFLAGS = -Wall -Wextra -pthread

TARGET = hw3

SRC = hw3.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
