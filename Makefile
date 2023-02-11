CC=gcc
CFLAGS=-Wall -Wextra -ggdb -std=gnu99

all: rserver

rserver: rserver.o optparser.o a3protos.o userlist.o roomlist.o
	$(CC) -o rserver rserver.o optparser.o a3protos.o userlist.o roomlist.o

server.o: rserver.c optparser.h a3protos.h
	$(CC) $(CFLAGS) -c rserver.c

a3protos.o: a3protos.c a3protos.h

optparser.o: optparser.c optparser.h

userlist.o: userlist.c userlist.h a3protos.h

roomlist.o: roomlist.c roomlist.h

clean:
	@echo "Removing object and exe files"
	rm -f *.o rserver

.PHONY : clean all

