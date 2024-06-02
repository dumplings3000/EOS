CC = gcc
CFLAGS = -Wall -Wextra

all: game guess

game: game.c
	$(CC) $(CFLAGS) -o game game.c

guess: guess.c
	$(CC) $(CFLAGS) -o guess guess.c

clean:
	rm -f game guess
