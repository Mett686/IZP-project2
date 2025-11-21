CC = cc
CFLAGS = -std=c11 -g -Wall -Wextra -Werror -pedantic
LDLIBS = -lm

flows: flows.c

test:
	./flows data.txt

clean:
	rm -f flows