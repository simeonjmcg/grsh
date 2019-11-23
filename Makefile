CC=gcc
CFLAGS=

make: src/grsh.c
	$(CC) -o build/grsh src/grsh.c
run: make
	build/grsh
