CC=gcc
CFLAGS=-Wall -Wextra -std=c99
LDFLAGS=-lreadline

dsh: dsh.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

.PHONY: clean run

clean:
	rm -f dsh


