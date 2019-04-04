CC      ?= clang
CXX     ?= clang++
CFLAGS  ?= -std=c89 -Wall -Wextra -pedantic -Werror
LDFLAGS ?=

CFLAGS  += $(addprefix -I,$(shell llvm-config --includedir))
LDFLAGS += $(shell llvm-config --ldflags --system-libs --libs core support analysis)

.PHONY: all clean

all: nocc

nocc: main.o
	${CXX} ${CFLAGS} -o $@ $^ ${LDFLAGS}

clean:
	${RM} nocc *.o
