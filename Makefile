
all: client

client: client.c
	gcc $^ -o $@

clean:
	rm -fv client
