CC      ?= clang
CFLAGS  ?= -std=c89 -Wall -Wextra -pedantic -Werror
LDFLAGS ?=

all: nocc

nocc: main.o
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}
