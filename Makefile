CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra -pthread
LDFLAGS = -pthread

.PHONY: all
all: nyuenc

nyuenc: nyuenc.o threadpool.o

nyuenc.o: nyuenc.c threadpool.h

threadpool.o: threadpool.c threadpool.h

.PHONY: clean
clean:
	rm -f *.o nyuenc
