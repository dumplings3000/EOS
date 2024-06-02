CC = gcc

CFLAGS = -Wall -Wextra -pthread

TARGET = hw2

SRC = hw2.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)