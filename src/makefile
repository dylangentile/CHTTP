CC := clang
CFLAGS := -g -std=c11 -I../include -Wall -Wextra -pedantic
RM := rm

OBJ := main.o CHTTP.o header_map.o hash_map.o socket.o

all: server

server: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

depend:
	$(CC) $(CFLAGS) -E -MM *.c > .depend

clean:
	$(RM) -f $(OBJ) server
	$(RM) -rf *.dSYM


include .depend
