COMMON_CFLAGS=-Wall -Wextra -Werror --std=c99 -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter

DEBUG_CFLAGS=-g

PROFILE_CFLAGS=
#PROFILE_CFLAGS+=-pg
#PROFILE_CFLAGS+=-fprofile-arcs #-ftest-coverage

OPTIMIZE_CFLAGS=
#OPTIMIZE_CFLAGS+=-DNO_GC_STATISTICS
#OPTIMIZE_CFLAGS+=-Os -DNDEBUG -DNO_CLEAR_ON_FREE

DEFINES_CFLAGS=
#DEFINES_CFLAGS+=-DEVAL_TRACING
#DEFINES_CFLAGS+=-DGC_REACHABILITY_VERIFICATION
#DEFINES_CFLAGS+=-DNO_CLEAR_ON_FREE

#DEFINES_CFLAGS+=-DALLOC_DEBUG
#DEFINES_CFLAGS+=-DCLEAR_ON_FREE
#DEFINES_CFLAGS+=-DGC_REFCOUNT_DEBUG
#DEFINES_CFLAGS+=-DGC_TRACING
#DEFINES_CFLAGS+=-DGC_VERBOSE
#DEFINES_CFLAGS+=-DNODE_GC_TRACING
#DEFINES_CFLAGS+=-DNODE_INCREMENTAL_FULL_GC
#DEFINES_CFLAGS+=-DNODE_INIT_TRACING
#DEFINES_CFLAGS+=-DNODE_NO_INCREMENTAL_GC


CFLAGS=${COMMON_CFLAGS} ${OPTIMIZE_CFLAGS} ${DEBUG_CFLAGS} ${PROFILE_CFLAGS} ${DEFINES_CFLAGS}

LDFLAGS=
#LDFLAGS+= -ldl -Wl,--export-dynamic # Linux
#LDFLAGS+= -pg -fprofile-arcs -lgcov
#LDFLAGS+=-lgcov


SO_LDFLAGS=-fPIC -dynamiclib -Wl,-undefined,dynamic_lookup # MacOSX
#SO_LDFLAGS=-fPIC -shared -lgcov -fprofile-arcs # Linux

EXECUTABLES=tokenize_test parse_test eval_test paren
OUTLIBS=base.so sio.so testutil.so

default: ${EXECUTABLES} ${OUTLIBS}

clean:
	rm *.o ${EXECUTABLES} ${OUTLIBS}

tokenize_test: tokenize_test.o token.o
	gcc ${LDFLAGS} -o $@ $^

parse_test: parse_test.o parse.o token.o node.o memory.o dlist.o
	gcc ${LDFLAGS} -o $@ $^

eval_test: eval_test.o builtins.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o dlist.o foreign_common.o frame.o
	gcc ${LDFLAGS} -o $@ $^

base.so: builtins_shared.o builtins.o
	gcc ${SO_LDFLAGS} -o $@ $^

sio.so: sio.o
	gcc ${SO_LDFLAGS} -o $@ $^

testutil.so: testutil.o
	gcc ${SO_LDFLAGS} -o $@ $^

paren: paren.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o dlist.o load_wrapper.o builtin_load.o foreign_common.o frame.o
	gcc ${LDFLAGS} -o $@ $^

builtins.o: builtins.c
	gcc ${CFLAGS} -c $< -fPIC

sio.o: sio.c
	gcc ${CFLAGS} -c $< -fPIC

testutil.o: testutil.c
	gcc ${CFLAGS} -c $< -fPIC

%.o: %.c
	gcc ${CFLAGS} -c $<
