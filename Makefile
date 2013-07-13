#CFLAGS=-DGC_REFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -g -Wall -Wextra -Werror --std=c99 -Wno-unused-function
#CFLAGS=-DGC_REFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_GC_TRACING -DGC_TRACING -g -Wall -Wextra -Werror --std=c99 -Wno-unused-function
#CFLAGS=-DGC_REFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_GC_TRACING -DGC_TRACING -g -Wall -Wextra -Werror --std=c99 -Wno-unused-function
#CFLAGS=-DNODE_INIT_TRACING -g -Wall -Wextra -Werror --std=c99 -Wno-unused-function
#CFLAGS=-DGC_REFCOUNT_DEBUG -DALLOC_DEBUG -DGC_TRACING -g -Wall -Wextra -Werror --std=c99 -Wno-unused-function
#CFLAGS=-DALLOC_DEBUG -DGC_REFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_GC_TRACING -DGC_TRACING -Wall -Wextra -Werror -g --std=c99 -O0 -Wno-unused-function
#CFLAGS=-Wall -Wextra -Werror -g --std=c99 -DCLEAR_ON_FREE -Wno-unused-function
CFLAGS=-Wall -Wextra -Werror -g --std=c99 -DEVAL_TRACING -DNODE_INCREMENTAL_FULL_GC -Wno-unused-function
#CFLAGS=-Wall -Wextra -Werror -g --std=c99 -DEVAL_TRACING -Wno-unused-function
#CFLAGS=-Wall -Wextra -Werror -DNDEBUG -Os --std=c99 -Wno-unused-function


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
	gcc -S ${CFLAGS} -c $<
	time gcc ${CFLAGS} -c $<
