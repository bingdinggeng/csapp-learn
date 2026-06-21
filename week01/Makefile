CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11

PROGRAMS = hello swap array string struct malloc_practice

all: $(PROGRAMS)

hello: hello.c
	$(CC) $(CFLAGS) $< -o $@

swap: swap.c
	$(CC) $(CFLAGS) $< -o $@

array: array.c
	$(CC) $(CFLAGS) $< -o $@

string: string.c
	$(CC) $(CFLAGS) $< -o $@

struct: struct.c
	$(CC) $(CFLAGS) $< -o $@

malloc_practice: malloc_practice.c
	$(CC) $(CFLAGS) $< -o $@

run: all
	./hello
	./swap
	./array
	./string
	./struct
	./malloc_practice

memcheck: malloc_practice
	valgrind --leak-check=full ./malloc_practice

clean:
	rm -f $(PROGRAMS)
