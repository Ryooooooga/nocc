CC       := clang
CXX      := clang++
CFLAGS   := -std=c99 -Wall -Wextra -pedantic -Werror -Wno-override-module -Wno-incompatible-library-redeclaration -Wno-int-to-void-pointer-cast
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -Werror -Wno-override-module -Wno-incompatible-library-redeclaration -Wno-int-to-void-pointer-cast
LDFLAGS  :=

CFLAGS   += $(addprefix -I,$(shell llvm-config --includedir))
CXXFLAGS += $(addprefix -I,$(shell llvm-config --includedir))
LDFLAGS  += $(shell llvm-config --ldflags --system-libs --libs core support analysis executionengine mcjit interpreter native)

.PHONY: all test clean

all: nocc test_nocc nocc_stage3

test: nocc test_nocc nocc_stage3
	./test_nocc .
	cmp -b nocc_stage2 nocc_stage3

nocc: main.o libnocc.a
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

test_nocc: test.o test_path.o test_vec.o test_map.o test_lexer.o test_preprocessor.o test_parser.o test_generator.o test_engine.o libnocc.a
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

libnocc.a: file.o generator.o lexer.o map.o parser.o path.o preprocessor.o sema.o scope_stack.o symbol.o type.o util.o vec.o
	${AR} rc $@ $^

nocc_stage2: file-2.ll generator-2.ll lexer-2.ll map-2.ll parser-2.ll path-2.ll preprocessor-2.ll symbol-2.ll sema-2.ll scope_stack-2.ll type-2.ll util-2.ll vec-2.ll main-2.ll
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

nocc_stage3: file-3.ll generator-3.ll lexer-3.ll map-3.ll parser-3.ll path-3.ll preprocessor-3.ll symbol-3.ll sema-3.ll scope_stack-3.ll type-3.ll util-3.ll vec-3.ll main-3.ll
	${CXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}

%.o: %.c *.h
	${CC} ${CFLAGS} -c -o $@ $<

%-2.ll: %.c *.h nocc
	./nocc $< > $@

%-3.ll: %.c *.h nocc_stage2
	./nocc_stage2 $< > $@

clean:
	${RM} nocc test_nocc nocc_stage* *.a *.o *.ll
