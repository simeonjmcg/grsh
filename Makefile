CC=gcc
CFLAGS=-Wall

make: src/grsh.c
	$(CC) -o build/grsh src/grsh.c
makedebug: src/grsh.c
	$(CC) -g -o build/grsh src/grsh.c
run: make
	build/grsh
debug: makedebug
	gdb build/grsh
