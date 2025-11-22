CC = cc
CFLAGS = -std=c11 -g -Wall -Wextra -Werror -pedantic
LDLIBS = -lm

flows: flows.c

test:
	./flows data.txt 2 1.0 1.0 1.0 1.0

clean:
	rm -f flows