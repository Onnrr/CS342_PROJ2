CFLAGS = -Wall -Wextra -pthread

all: mps

mps: mps.c
	gcc $(CFLAGS) -o mps mps.c -lm

clear:
	rm -f mps