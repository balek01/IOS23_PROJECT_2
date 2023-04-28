CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic  -pthread
OUTPUT=proj2

build: proj2.c proj2.h
	$(CC) $(CFLAGS) proj2.c -o $(OUTPUT)

clean:
	rm -f $(OUTPUT)