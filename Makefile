all: runtc.c
	gcc -g -Wall -o runtc runtc.c

run: all
	sudo ./runtc bash
