#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "parse.h"
#include "eval.h"
#include "environ_utils.h"
#include "builtin_load.h"

#define DEFINE_SPECIAL_MAKER(FUNC) \
static node_t *make_special_ ## FUNC (void) \
{ return node_special_func_new(SPECIAL_ ## FUNC); }

DEFINE_SPECIAL_MAKER(IF)
DEFINE_SPECIAL_MAKER(LAMBDA)
DEFINE_SPECIAL_MAKER(MK_CONT)
DEFINE_SPECIAL_MAKER(QUOTE)
DEFINE_SPECIAL_MAKER(DEF)
DEFINE_SPECIAL_MAKER(SET)
DEFINE_SPECIAL_MAKER(DEFINED)

#define ARR_LEN(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

foreign_assoc_t startenv[] = {
	{ "quote",    make_special_QUOTE },
	{ "if",       make_special_IF },
	{ "lambda",   make_special_LAMBDA },
	{ "call/cc",  make_special_MK_CONT },
	{ "def!",     make_special_DEF },
	{ "set!",     make_special_SET },
	{ "defined?", make_special_DEFINED },
};

void usage(char *name)
{
	printf("usage: %s <file>\n", name);
}

void environ_add_argv(node_t *env_handle, int argc, char *argv[])
{
	unsigned int len;
	int i, c;
	node_t *cnode, *lnode, *argv_head;

	argv_head = NULL;
	for(i = argc - 1; i >= 0; i--) {
		len = strlen(argv[i]);
		lnode = NULL;
		for(c = len - 1; c >= 0; c--) {
			cnode = node_value_new(argv[i][c]);
			lnode = node_cons_new(cnode, lnode);
		}
		argv_head = node_cons_new(lnode, argv_head);
	}
	environ_add(env_handle, node_symbol_new("ARGV"), argv_head);
}

int main(int argc, char *argv[])
{
	node_t *parse_result = NULL, *eval_result = NULL, *env_handle = NULL;
	eval_err_t eval_stat;
	parse_err_t parse_stat;
	char *remain;
	size_t i;
	int status = 0;
	char *infile;
	int infd = -1;
	char *filebuf = NULL, *bufcurs;
	size_t file_len = 0;
	struct stat sb;

	nodes_initialize();

	/* initialize environment */
	env_handle = node_handle_new(NULL);
	node_lockroot(env_handle);
	{
		node_t *name, *val;
		name = node_symbol_new("_load-lib");
		val = node_foreign_new(foreign_load);
		environ_add(env_handle, name, val);
	}
	environ_add_argv(env_handle, argc-1, argv+1); // NB: argv[0] is interp name
	environ_add_builtins(env_handle, startenv, ARR_LEN(startenv));

	if(argc < 2) {
		goto cleanup;
	}

	infile = argv[1];
	status = open(infile, O_RDONLY);
	if(status < 0) {
		fprintf(stderr, "error: cannot open() file %s: %s (errno=%d)\n",
		        infile, strerror(errno), errno);		
		status = -1;
		goto cleanup;
	}
	infd = status;

	status = fstat(infd, &sb);
	if(status < 0) {
		fprintf(stderr, "error: canno stat() file %s: %s (errno=%d)\n",
		        infile, strerror(errno), errno);		
		status = -1;
		goto cleanup;
	}
	file_len = sb.st_size;

	filebuf = mmap(NULL, file_len, PROT_READ, MAP_PRIVATE, infd, 0);
	if(filebuf == MAP_FAILED) {
		fprintf(stderr, "error: cannot mmap() file %s: %s (errno=%d)\n",
		        infile, strerror(errno), errno);		
		status = -1;
		goto cleanup;
	}

	for(bufcurs = filebuf;
	    bufcurs - filebuf < (ssize_t) file_len;
	    ) {
		parse_stat = parse(bufcurs, &bufcurs, &parse_result, NULL);
		if(parse_stat != PARSE_OK) {
			printf("parse error for: %s\n", bufcurs);
			printf("-- %s\n", parse_err_str(parse_stat));
			status = -1;
			goto cleanup;
		}
		if(parse_result) {
			node_lockroot(parse_result);
		}
		eval_stat = eval(parse_result, env_handle, &eval_result);
		if(eval_stat) {
			printf("eval error for: \n");
			node_print_pretty(eval_result, false);
			printf("\n");
			printf("-- %s\n", eval_err_str(eval_stat));
			status = -1;
			goto cleanup;
		}
	}
	node_print_pretty(eval_result, false);
	printf("\n");

cleanup:
	node_droproot(parse_result);
	node_droproot(eval_result);
	node_droproot(env_handle);
	if(filebuf != NULL && filebuf != MAP_FAILED) {
		munmap(filebuf, file_len);
	}
	if(infd >= 0) {
		close(infd);
	}
	if(getenv("PAREN_LEAK_CHECK")) {
		uintptr_t total_alloc, free_alloc;
		node_gc();
		node_gc();
		node_gc_statistics(&total_alloc, &free_alloc, NULL, NULL);
		if(total_alloc != free_alloc) {
			printf("warning: allocations remain at exit!\n");
		}
	}
	return status;
}
