# Makefile for building the simple HTTP server

CC=gcc
CFLAGS=-Wall -I/usr/local/include/quickjs
LDFLAGS=-L/usr/local/lib/quickjs -lquickjs

all: server

server: server.c
	$(CC) $(CFLAGS) server.c -o server $(LDFLAGS)

clean:
	rm -f server

