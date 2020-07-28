CC := clang
CFLAGS := -g -std=c11 -I../include -Wall -Wextra -pedantic
RM := rm

OBJ := CHTTP.o header_map.o hash_map.o socket.o

.PHONY: all clean

all: libchttp.a

libchttp.a: $(OBJ)
	ar -rc libchttp.a $(OBJ)

server: libchttp.a main.o
	$(CC) -L. -lchttp main.o -o server

depend:
	$(CC) $(CFLAGS) -E -MM *.c > .depend

clean:
	-$(RM) -f $(OBJ) server
	-$(RM) -rf *.dSYM
	-$(RM) main.o libchttp.a

include .depend
