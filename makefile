
.PHONY: all clean

all:
	touch src/.depend
	make -C src depend
	make -C src all
	mv src/server .

clean:
	make -C src clean
	rm server
