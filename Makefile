
BIN=client server unit_test

all: $(BIN)

init:
	@git submodule init
	@git submodule update

client: client.c linenoise/linenoise.c
	gcc -I./ $^ -o $@

server: server.c linenoise/linenoise.c
	gcc -I./ $^ -o $@

unit_test: unit_test.c
	gcc -I./ $^ -o $@


clean:
	rm -fv $(BIN) *.o
