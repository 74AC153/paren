COMMON_CFLAGS=-Wall -Wextra -Werror --std=c99 -Wno-unused-function

OPTIMIZE_CFLAGS=-g
#OPTIMIZE_CFLAGS=-Os -DNDEBUG

DEFINES_CFLAGS=
#DEFINES_CFLAGS+=-DALLOC_DEBUG
#DEFINES_CFLAGS+=-DCLEAR_ON_FREE
DEFINES_CFLAGS+=-DEVAL_TRACING
DEFINES_CFLAGS+=-DGC_REACHABILITY_VERIFICATION
#DEFINES_CFLAGS+=-DGC_REFCOUNT_DEBUG
#DEFINES_CFLAGS+=-DGC_TRACING
#DEFINES_CFLAGS+=-DGC_VERBOSE
#DEFINES_CFLAGS+=-DNODE_GC_TRACING
#DEFINES_CFLAGS+=-DNODE_INCREMENTAL_FULL_GC
#DEFINES_CFLAGS+=-DNODE_INIT_TRACING
#DEFINES_CFLAGS+=-DNODE_NO_INCREMENTAL_GC


CFLAGS=${COMMON_CFLAGS} ${OPTIMIZE_CFLAGS} ${DEFINES_CFLAGS}

LDFLAGS=

default: tokenize_test parse_test eval_test

clean:
	rm *.o *.s

tokenize_test: tokenize_test.o token.o
	gcc ${LDFLAGS} -o tokenize_test tokenize_test.o token.o

parse_test: parse_test.o parse.o token.o node.o memory.o dlist.o
	gcc ${LDFLAGS} -o parse_test parse_test.o parse.o token.o node.o memory.o dlist.o

eval_test: eval_test.o builtins.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o dlist.o
	gcc ${LDFLAGS} -o eval_test eval_test.o builtins.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o dlist.o

%.o: %.c
	gcc -S ${CFLAGS} -c $<
	time gcc ${CFLAGS} -c $<
