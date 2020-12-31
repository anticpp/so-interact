Interaction tool(using `linenoise`) to control every step of socket api. This is a good tool to test low primitive socket api, program will stop and wait until you interact with a command.

## Build

```
make init
make all
```

## Unit test

```
./unit_test
```

## Usage

```
echo "help" | ./client
```

## TODO

- Use unit framework
- server mode
- Fix mem leak at line `client.c:141`
    + Done
- Fix mem leak for `client.c`'s `line`
    + Done
- Support v6
- Non-blocking
