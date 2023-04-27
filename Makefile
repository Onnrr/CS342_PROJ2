CFLAGS = -Wall -Wextra -pthread

all: mps

mps: mps.c
	gcc $(CFLAGS) -o mps mps.c -lm -ggdb3

clear:
	rm -f mps