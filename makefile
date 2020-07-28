
.PHONY: all clean

all: lib

lib:
	touch src/.depend
	make -C src depend
	make -C src all

server: lib
	make -C src server
	mv src/server .

clean:
	make -C src clean
	rm server
