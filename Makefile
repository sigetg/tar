tar: tar.o
	gcc -std=c99 -Wall -g -o tar tar.o

tar.o: tar.c
	gcc -std=c99 -Wall -g -c tar.c

clean:
	rm -rf tar tar.o
