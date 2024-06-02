CC = gcc

CFLAGS = -Wall -Wextra -pthread

TARGET1 = server
TARGET2 = client

SRC1 = server.c
SRC2 = client.c

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(SRC1)
	$(CC) $(CFLAGS) -o $@ $^
$(TARGET2): $(SRC2)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET1) $(TARGET2)
