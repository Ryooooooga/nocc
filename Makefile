CC       := clang
CXX      := clang++
CFLAGS   := -std=c99 -Wall -Wextra -pedantic -Werror
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -Werror
LDFLAGS  :=

CFLAGS   += $(addprefix -I,$(shell llvm-config --includedir))
CXXFLAGS += $(addprefix -I,$(shell llvm-config --includedir))
LDFLAGS  += $(shell llvm-config --ldflags --system-libs --libs core support analysis)

.PHONY: all test clean

all: nocc test_nocc

test: test_nocc
	./test_nocc

nocc: main.o libnocc.a
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

test_nocc: test.o test_vec.o test_lexer.o test_parser.o test_generator.o libnocc.a
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

libnocc.a: generator.o lexer.o parser.o vec.o
	${AR} rcv $@ $^

clean:
	${RM} nocc test_nocc *.a *.o
