CC       := clang
CXX      := clang++
CFLAGS   := -std=c99 -Wall -Wextra -pedantic -Werror
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -Werror
LDFLAGS  :=

CFLAGS   += $(addprefix -I,$(shell llvm-config --includedir))
CXXFLAGS += $(addprefix -I,$(shell llvm-config --includedir))
LDFLAGS  += $(shell llvm-config --ldflags --system-libs --libs core support analysis executionengine mcjit interpreter native)

.PHONY: all test clean

all: nocc test_nocc nocc_stage2

test: nocc test_nocc nocc_stage2
	./test_nocc .

nocc: main.o libnocc.a
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

test_nocc: test.o test_path.o test_vec.o test_map.o test_lexer.o test_preprocessor.o test_parser.o test_generator.o test_engine.o libnocc.a
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

libnocc.a: file.o generator.o lexer.o map.o parser.o path.o preprocessor.o sema.o scope_stack.o type.o util.o vec.o
	${AR} rc $@ $^

nocc_stage2: file-2.ll generator-2.ll lexer-2.ll map-2.ll parser-2.ll path-2.ll preprocessor-2.ll sema-2.ll scope_stack-2.ll type-2.ll util-2.ll vec-2.ll main-2.ll
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

%.o: %.c nocc.h
	${CC} ${CFLAGS} -c -o $@ $<

%-2.ll: %.c nocc.h nocc
	./nocc $< 2> $@

clean:
	${RM} nocc test_nocc *.a *.o *.ll
