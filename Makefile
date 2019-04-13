CC       := clang
CXX      := clang++
CFLAGS   := -std=c99 -Wall -Wextra -pedantic -Werror
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -Werror
LDFLAGS  :=

CFLAGS   += $(addprefix -I,$(shell llvm-config --includedir))
CXXFLAGS += $(addprefix -I,$(shell llvm-config --includedir))
LDFLAGS  += $(shell llvm-config --ldflags --system-libs --libs core support analysis executionengine mcjit interpreter native)

.PHONY: all test clean

all: nocc test_nocc

test: nocc test_nocc
	./test_nocc

nocc: main.o libnocc.a
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

test_nocc: test.o test_vec.o test_map.o test_lexer.o test_parser.o test_generator.o test_engine.o libnocc.a
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

libnocc.a: file.o generator.o lexer.o map.o parser.o sema.o scope_stack.o type.o util.o vec.o
	${AR} rc $@ $^

%.o: %.c nocc.h
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	${RM} nocc test_nocc *.a *.o
