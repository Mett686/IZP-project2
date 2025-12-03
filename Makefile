CC = cc
CFLAGS = -std=c11 -g -Wall -Wextra -Werror -pedantic
LDLIBS = -lm

flows: flows.c

test: flows
	./flows data.txt 2 1.0 1.0 1.0 1.0

test1: flows
	./flows data.txt

valgrind: flows
	valgrind ./flows ./data.txt 2 1 1 1 1

clean:
	rm -f flows
	rm -f out*.txt
	rm -f data_*.txt