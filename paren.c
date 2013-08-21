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
#include "environ.h"
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
DEFINE_SPECIAL_MAKER(EVAL)

#define ARR_LEN(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

foreign_assoc_t startenv[] = {
	{ "quote",    make_special_QUOTE },
	{ "if",       make_special_IF },
	{ "lambda",   make_special_LAMBDA },
	{ "call/cc",  make_special_MK_CONT },
	{ "def!",     make_special_DEF },
	{ "set!",     make_special_SET },
	{ "defined?", make_special_DEFINED },
	{ "eval",     make_special_EVAL },
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

typedef struct {
	int fd;
	void *buf;
	size_t len;
	int err;
} filemap_info_t;

int map_file(filemap_info_t *info, char *path)
{
	struct stat sb;
	int status = 0;

	info->fd = open(path, O_RDONLY);
	if(info->fd < 0) {
		status = -1;
		info->err = errno;
		goto finish;
	}

	status = fstat(info->fd, &sb);
	if(status < 0) {
		status = -2;
		info->err = errno;
		goto finish;
	}
	info->len = sb.st_size;

	info->buf = mmap(NULL, info->len, PROT_READ, MAP_PRIVATE, info->fd, 0);
	if(info->buf == MAP_FAILED) {
		status = -3;
		info->err = errno;
		goto finish;
	}

finish:
	return status;
}

void unmap_file(filemap_info_t *info)
{
	if(info->buf != NULL && info->buf != MAP_FAILED) {
		munmap(info->buf, info->len);
	}
	if(info->fd >= 0) {
		close(info->fd);
	}
}

int main(int argc, char *argv[])
{
	node_t *parse_result, *eval_in_hdl, *eval_out_hdl, *env_handle;
	eval_err_t eval_stat;
	parse_err_t parse_stat;
	char *remain;
	size_t i;
	int status = 0;
	char *bufcurs;

	filemap_info_t info;

	if(argc < 2) {
		goto cleanup;
	}

	nodes_initialize();

	/* initialize environment */
	eval_in_hdl = node_lockroot(node_handle_new(NULL));
	eval_out_hdl = node_lockroot(node_handle_new(NULL));
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

	status = map_file(&info, argv[1]);
	switch(status) {
	default: break;
	case -1:
		fprintf(stderr, "error: cannot open() file %s: %s (errno=%d)\n",
		        argv[1], strerror(info.err), info.err);		
		goto cleanup;
	case -2:
		fprintf(stderr, "error: canno stat() file %s: %s (errno=%d)\n",
		        argv[1], strerror(info.err), info.err);		
		goto cleanup;
	case -3:
		fprintf(stderr, "error: cannot mmap() file %s: %s (errno=%d)\n",
		        argv[1], strerror(info.err), info.err);		
		goto cleanup;
	}

	for(bufcurs = info.buf;
	    (uintptr_t) bufcurs - (uintptr_t) info.buf < info.len;
	    ) {
		parse_stat = parse(bufcurs, &bufcurs, &parse_result, NULL);
		if(parse_stat != PARSE_OK) {
			printf("parse error for: %s\n", bufcurs);
			printf("-- %s\n", parse_err_str(parse_stat));
			status = -1;
			goto cleanup;
		}
		node_handle_update(eval_in_hdl, parse_result);
		eval_stat = eval(eval_in_hdl, env_handle, eval_out_hdl);
		if(eval_stat) {
			printf("eval error for: \n");
			node_print_pretty(node_handle(eval_out_hdl), false);
			printf("\n");
			printf("-- %s\n", eval_err_str(eval_stat));
			status = -1;
			goto cleanup;
		}
		node_handle_update(eval_in_hdl, NULL);
		node_handle_update(eval_out_hdl, NULL);
	}

cleanup:
	if(getenv("PAREN_FINALENV")) {
		environ_print(env_handle);
	}

	node_droproot(eval_in_hdl);
	node_droproot(eval_out_hdl);
	node_droproot(env_handle);
	unmap_file(&info);

	if(getenv("PAREN_MEMSTAT")) {
		uintptr_t total_alloc, total_free;
		unsigned long long gc_cycles, gc_iters;
		node_gc_statistics(&total_alloc, &total_free, &gc_iters, &gc_cycles);
		printf("total alloc: %llu total free: %llu iters: %llu cycles: %llu\n",
		       (unsigned long long) total_alloc,
		       (unsigned long long) total_free,
		       (unsigned long long) gc_iters,
		       (unsigned long long) gc_cycles);
	}

	if(getenv("PAREN_LEAK_CHECK")) {
		uintptr_t total_alloc, free_alloc;
		node_gc();
		node_gc();
		node_gc_statistics(&total_alloc, &free_alloc, NULL, NULL);
		if(total_alloc != free_alloc) {
			printf("warning: %llu allocations remain at exit!\n",
			       (unsigned long long) (total_alloc - free_alloc));
		}
	}

	if(getenv("PAREN_DUMPMEM")) {
		node_gc_print_state();
	}
	return status;
}
