all: build

build:
	gcc -Wall -Wpedantic -ggdb -std=c99 -D _POSIX_C_SOURCE=200809L main.c list.o sender.c receiver.c -lpthread -o s-talk

clean: 
	rm -f s-talk *.o