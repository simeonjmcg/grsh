CC=gcc
CFLAGS=

makegrsh: src/grsh.c
	$(CC) -o build/grsh src/grsh.c
