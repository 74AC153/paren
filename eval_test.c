#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "parse.h"
#include "eval.h"
#include "environ_utils.h"
#include "builtins.h"
#include "bufstream.h"
#include "fdstream.h"
#include "dbgtrace.h"
#include "malloc_wrapper.h"

#if defined(NO_GC_FREELIST)
#include "freemem_cache.h"
#endif

#define DEFINE_FOREIGN_FACTORY(FUNC) \
static node_t *make_ ## FUNC (memory_state_t *ms) \
	{ return node_foreign_new(ms, FUNC); }

DEFINE_FOREIGN_FACTORY(foreign_nil_p)
DEFINE_FOREIGN_FACTORY(foreign_val_p)
DEFINE_FOREIGN_FACTORY(foreign_sym_p)
DEFINE_FOREIGN_FACTORY(foreign_cons_p)
DEFINE_FOREIGN_FACTORY(foreign_func_p)
DEFINE_FOREIGN_FACTORY(foreign_car)
DEFINE_FOREIGN_FACTORY(foreign_cdr)
DEFINE_FOREIGN_FACTORY(foreign_mksym)
DEFINE_FOREIGN_FACTORY(foreign_splsym)
DEFINE_FOREIGN_FACTORY(foreign_cons)
DEFINE_FOREIGN_FACTORY(foreign_smeq_p)
DEFINE_FOREIGN_FACTORY(foreign_eq_p)
DEFINE_FOREIGN_FACTORY(foreign_lt_p)
DEFINE_FOREIGN_FACTORY(foreign_gt_p)
DEFINE_FOREIGN_FACTORY(foreign_ult_p)
DEFINE_FOREIGN_FACTORY(foreign_ugt_p)
DEFINE_FOREIGN_FACTORY(foreign_addc_p)
DEFINE_FOREIGN_FACTORY(foreign_add)
DEFINE_FOREIGN_FACTORY(foreign_sub)
DEFINE_FOREIGN_FACTORY(foreign_mul)
DEFINE_FOREIGN_FACTORY(foreign_div)
DEFINE_FOREIGN_FACTORY(foreign_rem)
DEFINE_FOREIGN_FACTORY(foreign_b_and)
DEFINE_FOREIGN_FACTORY(foreign_b_or)
DEFINE_FOREIGN_FACTORY(foreign_b_nand)
DEFINE_FOREIGN_FACTORY(foreign_b_xor)
DEFINE_FOREIGN_FACTORY(foreign_b_shl)
DEFINE_FOREIGN_FACTORY(foreign_b_shr)
DEFINE_FOREIGN_FACTORY(foreign_b_shra)

#define DEFINE_SPECIAL_FACTORY(FUNC) \
static node_t *make_special_ ## FUNC (memory_state_t *ms) \
	{ return node_special_func_new(ms, SPECIAL_ ## FUNC); }

DEFINE_SPECIAL_FACTORY(IF)
DEFINE_SPECIAL_FACTORY(LAMBDA)
DEFINE_SPECIAL_FACTORY(MK_CONT)
DEFINE_SPECIAL_FACTORY(QUOTE)
DEFINE_SPECIAL_FACTORY(DEF)
DEFINE_SPECIAL_FACTORY(SET)
DEFINE_SPECIAL_FACTORY(DEFINED)
DEFINE_SPECIAL_FACTORY(EVAL)

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

	{ "nil?",     make_foreign_nil_p },
	{ "value?",   make_foreign_val_p },
	{ "sym?",     make_foreign_sym_p },
	{ "cons?",    make_foreign_cons_p },
	{ "func?",    make_foreign_func_p },

	{ "car",      make_foreign_car },
	{ "cdr",      make_foreign_cdr },
	{ "makesym",  make_foreign_mksym },
	{ "splitsym", make_foreign_splsym },
	{ "cons",     make_foreign_cons },

	{ "symeq?",   make_foreign_smeq_p },
	{ "eq?",      make_foreign_eq_p },
	{ "lt?",      make_foreign_lt_p },
	{ "gt?",      make_foreign_gt_p },
	{ "ult?",     make_foreign_ult_p },
	{ "ugt?",     make_foreign_ugt_p },

	{ "addc",     make_foreign_addc_p },
	{ "add",      make_foreign_add },
	{ "sub",      make_foreign_sub },
	{ "mul",      make_foreign_mul },
	{ "div",      make_foreign_div },
	{ "rem",      make_foreign_rem },

	{ "b_and",    make_foreign_b_and },
	{ "b_or",     make_foreign_b_or },
	{ "b_nand",   make_foreign_b_nand },
	{ "b_xor",    make_foreign_b_xor },
	{ "b_shl",    make_foreign_b_shl },
	{ "b_shr",    make_foreign_b_shr },
	{ "b_shra",   make_foreign_b_shra },
};

int main(int argc, char *argv[])
{
	node_t *eval_in_hdl, *eval_out_hdl;
	eval_err_t eval_stat;
	parse_err_t parse_stat;

	char *remain;
	size_t i;
	bufstream_t bs;
	stream_t *stream;
	tok_state_t ts;
	memory_state_t ms;
	node_t *env_handle;

	dbgtrace_setstream(g_stream_stdout);
	dbgtrace_enable( 0
	               //| TC_MEM_ALLOC
	               //| TC_GC_TRACING
	               //| TC_MEM_RC
	               //| TC_GC_VERBOSE
	               //| TC_NODE_GC
	               //| TC_NODE_INIT
	               //| TC_EVAL
	               //| TC_FMC_ALLOC
	               );

	node_memstate_init(&ms, libc_malloc_wrap, libc_free_wrap, NULL);

	/* setup environment */
	eval_in_hdl = node_lockroot(node_handle_new(&ms, NULL));
	eval_out_hdl = node_lockroot(node_handle_new(&ms, NULL));
	printf("*** initialize environment ***\n");
	env_handle = node_handle_new(&ms, NULL);
	node_lockroot(env_handle);
	environ_add_builtins(env_handle, startenv, ARR_LEN(startenv));


	/* parse + eval argv */
	for(i = 1; i < (unsigned) argc; i++) {
		stream = bufstream_init(&bs, argv[i], strlen(argv[i]));
		tok_state_init(&ts, stream);
		printf("*** parse %s ***\n", argv[i]);
		parse_stat = parse(&ms, &ts, eval_in_hdl);
		if(parse_stat != PARSE_OK) {
			printf("parse error line %llu char %llu (offset %llu)\n",
			       (unsigned long long) tok_state_line(&ts),
			       (unsigned long long) tok_state_linechr(&ts),
			       (unsigned long long) tok_state_offset(&ts));
			printf("-- %s\n", parse_err_str(parse_stat));
			continue;
		}

		printf("parse result: %p\n", node_handle(eval_in_hdl));
		node_print_pretty_stream(g_stream_stdout,
		                         node_handle(eval_in_hdl), false);
		printf("\n");

		printf("start environment:\n");
		environ_print(env_handle, g_stream_stdout);

		printf("*** eval ***\n");
		eval_stat = eval(&ms, env_handle, eval_in_hdl, eval_out_hdl);

		if(eval_stat) {
			printf("eval error for: \n");
			node_print_pretty_stream(g_stream_stdout, 
			                         node_handle(eval_out_hdl), false);
			printf("\n");
			printf("%s\n", eval_err_str(eval_stat));
			return -1;
		}
		printf("eval result: %p\n", node_handle(eval_out_hdl));
		node_print_pretty_stream(g_stream_stdout,
		                         node_handle(eval_out_hdl), false);
		printf("\n");

		printf("*** releasing parse result %p ***\n", node_handle(eval_in_hdl));
		node_handle_update(eval_in_hdl, NULL);

		printf("*** releasing eval result %p ***\n", node_handle(eval_out_hdl));
		node_handle_update(eval_out_hdl, NULL);
	}

	printf("*** result environment %p ***\n", env_handle);
	environ_print(env_handle, g_stream_stdout);

	printf("*** release toplevel environment %p ***\n", env_handle);
	node_droproot(eval_in_hdl);
	node_droproot(eval_out_hdl);
	node_droproot(env_handle);
	
	printf("*** cleanup ***\n");

	memory_gc_print_state(&ms, g_stream_stdout);

	printf("*** run gc ***\n");
	memory_gc_cycle(&ms);
	printf("*** run gc ***\n");
	memory_gc_cycle(&ms);

	printf("*** final state ***\n");
	memory_gc_print_state(&ms, g_stream_stdout);

	memory_state_reset(&ms);

	return 0;
}
