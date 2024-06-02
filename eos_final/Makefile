CC = gcc

CFLAGS = -Wall -Wextra -pthread

TARGET = server

SRC = server.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)