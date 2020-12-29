
BIN=client unit_test

all: $(BIN)

init:
	@git module init
	@git module update

client: client.c linenoise/linenoise.c
	gcc -I./ $^ -o $@

unit_test: unit_test.c
	gcc -I./ $^ -o $@


clean:
	rm -fv $(BIN) *.o
