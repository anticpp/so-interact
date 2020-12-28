
all: client

CFLAGS=-DDEBUG

client: client.c linenoise/linenoise.c
	gcc -I./ $^ -o $@

clean:
	rm -fv client *.o
