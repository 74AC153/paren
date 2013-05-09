CFLAGS=-DREFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_INCREMENTAL_GC -g -Wall -Wextra -Werror --std=c99
#CFLAGS=-DREFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_INCREMENTAL_GC -DNODE_GC_TRACING -DGC_TRACING -g -Wall -Wextra -Werror --std=c99
#CFLAGS=-DREFCOUNT_DEBUG -DALLOC_DEBUG -DNODE_INIT_TRACING -DNODE_GC_TRACING -DGC_TRACING -g -Wall -Wextra -Werror --std=c99
#CFLAGS=-DREFCOUNT_DEBUG -DALLOC_DEBUG -DGC_TRACING -g -Wall -Wextra -Werror --std=c99
#CFLAGS=-Wall -Wextra -Werror -g --std=c99
#CFLAGS=-Wall -Wextra -Werror -Os


default: tokenize_test parse_test eval_test

clean:
	rm *.o

tokenize_test: tokenize_test.o token.o
	gcc -o tokenize_test tokenize_test.o token.o

parse_test: parse_test.o parse.o token.o node.o memory.o
	gcc -o parse_test parse_test.o parse.o token.o node.o memory.o

eval_test: eval_test.o builtins.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o
	gcc -o eval_test eval_test.o builtins.o eval.o parse.o token.o node.o environ.o environ_utils.o eval_err.o memory.o

builtins.o: builtins.c builtins.h
	gcc ${CFLAGS} -c builtins.c

environ.o: environ.c environ.h
	gcc ${CFLAGS} -c environ.c

eval.o: eval.c eval.h
	gcc ${CFLAGS} -c eval.c

parse.o: parse.c parse.h
	gcc ${CFLAGS} -c parse.c

token.o: token.c token.h
	gcc ${CFLAGS} -c token.c

node.o: node.c node.h dlist.h
	gcc ${CFLAGS} -c node.c

tokenize_test.o: tokenize_test.c
	gcc ${CFLAGS} -c tokenize_test.c

parse_test.o: parse_test.c
	gcc ${CFLAGS} -c parse_test.c

eval_test.o: eval_test.c
	gcc ${CFLAGS} -c eval_test.c

environ_utils.o: environ_utils.c environ_utils.h
	gcc ${CFLAGS} -c environ_utils.c

eval_err.o: eval_err.c eval_err.h
	gcc ${CFLAGS} -c eval_err.c

memory.o: memory.c memory.h
	gcc ${CFLAGS} -c memory.c
