COMMON_CFLAGS=-Wall -Wextra -Werror --std=c99 -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter

OPTIMIZE_CFLAGS=-g
#OPTIMIZE_CFLAGS=-Os -DNDEBUG -DNO_GC_STATISTICS -DNO_CLEAR_ON_FREE

DEFINES_CFLAGS=
DEFINES_CFLAGS+=-DEVAL_TRACING
DEFINES_CFLAGS+=-DGC_REACHABILITY_VERIFICATION
DEFINES_CFLAGS+=-DNO_CLEAR_ON_FREE
#DEFINES_CFLAGS+=-DALLOC_DEBUG
#DEFINES_CFLAGS+=-DCLEAR_ON_FREE
#DEFINES_CFLAGS+=-DGC_REFCOUNT_DEBUG
#DEFINES_CFLAGS+=-DGC_TRACING
#DEFINES_CFLAGS+=-DGC_VERBOSE
#DEFINES_CFLAGS+=-DNODE_GC_TRACING
#DEFINES_CFLAGS+=-DNODE_INCREMENTAL_FULL_GC
#DEFINES_CFLAGS+=-DNODE_INIT_TRACING
#DEFINES_CFLAGS+=-DNODE_NO_INCREMENTAL_GC


CFLAGS=${COMMON_CFLAGS} ${OPTIMIZE_CFLAGS} ${DEFINES_CFLAGS}

LDFLAGS=

SO_LDFLAGS=-fPIC -dynamiclib -Wl,-undefined,dynamic_lookup

EXECUTABLES=tokenize_test parse_test eval_test paren
OUTLIBS=builtins_shared.so

default: ${EXECUTABLES} ${OUTLIBS}

clean:
	rm *.o ${EXECUTABLES} ${OUTLIBS}

tokenize_test: tokenize_test.o token.o
	gcc ${LDFLAGS} -o $@ $^

parse_test: parse_test.o parse.o token.o node.o memory.o dlist.o
	gcc ${LDFLAGS} -o $@ $^

eval_test: eval_test.o builtins.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o dlist.o
	gcc ${LDFLAGS} -o $@ $^

builtins_shared.so: builtins_shared.o builtins.o
	gcc ${SO_LDFLAGS} -o $@ $^

paren: paren.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o dlist.o load_wrapper.o builtin_load.o
	gcc ${LDFLAGS} -o $@ $^

%.o: %.c
	gcc ${CFLAGS} -c $<
