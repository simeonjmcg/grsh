CC=gcc
CFLAGS=-Wall

make: src/grsh.c src/strtok_quo.c
	$(CC) -o build/grsh src/grsh.c src/strtok_quo.c
makedebug: src/grsh.c src/strtok_quo.c
	$(CC) -g -o build/grsh src/grsh.c src/strtok_quo.c
run: make
	build/grsh
debug: makedebug
	gdb build/grsh
