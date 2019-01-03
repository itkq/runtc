all: runtc.c
	gcc -g -Wall -o runtc runtc.c

run: all
	sudo ./runtc bash

init:
	./export-bionic

clean:
	rm -f runtc *.tar
