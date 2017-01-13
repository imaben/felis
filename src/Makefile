# Makefile for Auto Deploy
C = clang
CFLAGS = -g -c `pkg-config --cflags libevent`
LDFLAGS = -lm -lpthread `pkg-config --libs libevent`

all: felis.c
	$(CC) -o daemon.o daemon.c $(CFLAGS)
	$(CC) -o log.o log.c $(CFLAGS)
	$(CC) -o felis.o felis.c $(CFLAGS)
	$(CC) -o felis felis.o daemon.o log.o $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf felis
