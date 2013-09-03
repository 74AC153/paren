#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "parse.h"
#include "eval.h"
#include "environ_utils.h"
#include "environ.h"
#include "builtin_load.h"
#include "foreign_common.h"

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

int main(int argc, char *argv[])
{
	node_t *env_handle = NULL, *ARGV;
	int status = 0;

	if(argc < 2) {
		goto cleanup;
	}

	nodes_initialize();

	/* initialize environment */
	env_handle = node_lockroot(node_handle_new(NULL));

	{
		node_t *name, *val;
		name = node_symbol_new("_load-lib");
		val = node_foreign_new(foreign_loadlib);
		environ_add(env_handle, name, val);

		name = node_symbol_new("_read-eval");
		val = node_foreign_new(foreign_read_eval);
		environ_add(env_handle, name, val);

		// NB: argv[0] is interp name
		ARGV = generate_argv(argc-1, argv+1); 
		environ_add(env_handle, node_symbol_new("ARGV"), ARGV);

		environ_add_builtins(env_handle, startenv, ARR_LEN(startenv));
	}

	{
		node_t *eval_in_hdl, *eval_out_hdl;
		eval_err_t eval_stat;

		eval_in_hdl = node_lockroot(node_handle_new(NULL));
		eval_out_hdl = node_lockroot(node_handle_new(NULL));

		/* bootstrap routine:
		  ( _read-eval ( quote ARGV[0] ) )

		  ( _read-eval . ( ( quote . ( ARGV[0] . () ) ) . () ) )
		*/
		{
			node_t *quote_call;

			quote_call = node_cons_new(
			  node_symbol_new("quote"),
			  node_cons_new(
			    node_cons_car(ARGV),
			    NULL
			  )
			);

			node_handle_update(eval_in_hdl,
			  node_cons_new(
			    node_symbol_new("_read-eval"),
			    node_cons_new(
			      quote_call,
			      NULL
			    )
			  )
			);
		}

		eval_stat = eval(eval_in_hdl, env_handle, eval_out_hdl);
		if(eval_stat != EVAL_OK) {
			status = eval_stat;
		}

		node_droproot(eval_in_hdl);
		node_droproot(eval_out_hdl);
	}

cleanup:
	if(getenv("PAREN_FINALENV")) {
		environ_print(env_handle);
	}

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

	nodes_reset();

	return status;
}
