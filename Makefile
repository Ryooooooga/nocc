CC      ?= clang
CXX     ?= clang++
CFLAGS  ?= -std=c89 -Wall -Wextra -pedantic -Werror
LDFLAGS ?=

CFLAGS  += $(addprefix -I,$(shell llvm-config --includedir))
LDFLAGS += $(shell llvm-config --ldflags --system-libs --libs core support analysis)

.PHONY: all test clean

all: nocc test_nocc

test: test_nocc
	./test_nocc

nocc: main.o libnocc.a
	${CXX} ${CFLAGS} -o $@ $^ ${LDFLAGS}

test_nocc: test.o test_lexer.o test_vec.o libnocc.a
	${CXX} ${CFLAGS} -o $@ $^ ${LDFLAGS}

libnocc.a: lexer.o vec.o
	${AR} rcv $@ $^

clean:
	${RM} nocc test_nocc *.a *.o
