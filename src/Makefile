# Makefile for Auto Deploy
C = clang
LIBDFA = ./dfa
CFLAGS = -g -c `pkg-config --cflags libevent` -I$(LIBDFA)
LDFLAGS = -lm -lpthread `pkg-config --libs libevent`

all: felis.c
	$(CC) -o daemon.o daemon.c $(CFLAGS)
	$(CC) -o log.o log.c $(CFLAGS)
	$(CC) -o server.o server.c $(CFLAGS)
	$(CC) -o dict.o dict.c $(CFLAGS)
	$(CC) -o felis.o felis.c $(CFLAGS)
	$(CC) -o felis felis.o daemon.o log.o server.o dict.o $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf felis
