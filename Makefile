#CFLAGS=-DREFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_INCREMENTAL_GC -g -Wall -Wextra -Werror --std=c99
#CFLAGS=-DREFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_INCREMENTAL_GC -DNODE_GC_TRACING -DGC_TRACING -g -Wall -Wextra -Werror --std=c99
#CFLAGS=-DREFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_GC_TRACING -DGC_TRACING -g -Wall -Wextra -Werror --std=c99
#CFLAGS=-DREFCOUNT_DEBUG -DALLOC_DEBUG -DGC_TRACING -g -Wall -Wextra -Werror --std=c99
#CFLAGS=-DALLOC_DEBUG -DNODE_INCREMENTAL_GC -DGC_REFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_GC_TRACING -DGC_TRACING -Wall -Wextra -Werror -g --std=c99 -O0
CFLAGS=-Wall -Wextra -Werror -g --std=c99
#CFLAGS=-Wall -Wextra -Werror -DNDEBUG -Os --std=c99


default: tokenize_test parse_test eval_test

clean:
	rm *.o *.s

tokenize_test: tokenize_test.o token.o
	gcc -o tokenize_test tokenize_test.o token.o

parse_test: parse_test.o parse.o token.o node.o memory.o dlist.o
	gcc -o parse_test parse_test.o parse.o token.o node.o memory.o dlist.o

eval_test: eval_test.o builtins.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o dlist.o
	gcc -o eval_test eval_test.o builtins.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o dlist.o

%.o: %.c
	#gcc ${CFLAGS} -S $<
	gcc ${CFLAGS} -c $<
