#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "parse.h"
#include "eval.h"
#include "environ_utils.h"
#include "builtins.h"

#define DEFINE_FOREIGN_MAKER(FUNC) \
static node_t *make_ ## FUNC (void) \
	{ return node_foreign_new(FUNC); }

DEFINE_FOREIGN_MAKER(foreign_nil_p)
DEFINE_FOREIGN_MAKER(foreign_value_p)
DEFINE_FOREIGN_MAKER(foreign_sym_p)
DEFINE_FOREIGN_MAKER(foreign_cons_p)
DEFINE_FOREIGN_MAKER(foreign_func_p)
DEFINE_FOREIGN_MAKER(foreign_car)
DEFINE_FOREIGN_MAKER(foreign_cdr)
DEFINE_FOREIGN_MAKER(foreign_makesym)
DEFINE_FOREIGN_MAKER(foreign_splitsym)
DEFINE_FOREIGN_MAKER(foreign_cons)
DEFINE_FOREIGN_MAKER(foreign_symeq_p)
DEFINE_FOREIGN_MAKER(foreign_eq_p)
DEFINE_FOREIGN_MAKER(foreign_lt_p)
DEFINE_FOREIGN_MAKER(foreign_gt_p)
DEFINE_FOREIGN_MAKER(foreign_ult_p)
DEFINE_FOREIGN_MAKER(foreign_ugt_p)
DEFINE_FOREIGN_MAKER(foreign_addc_p)
DEFINE_FOREIGN_MAKER(foreign_add)
DEFINE_FOREIGN_MAKER(foreign_sub)
DEFINE_FOREIGN_MAKER(foreign_mul)
DEFINE_FOREIGN_MAKER(foreign_div)
DEFINE_FOREIGN_MAKER(foreign_rem)
DEFINE_FOREIGN_MAKER(foreign_bit_and)
DEFINE_FOREIGN_MAKER(foreign_bit_or)
DEFINE_FOREIGN_MAKER(foreign_bit_nand)
DEFINE_FOREIGN_MAKER(foreign_bit_xor)
DEFINE_FOREIGN_MAKER(foreign_bit_shl)
DEFINE_FOREIGN_MAKER(foreign_bit_shr)
DEFINE_FOREIGN_MAKER(foreign_bit_shra)

#define DEFINE_SPECIAL_MAKER(FUNC) \
static node_t *make_special_ ## FUNC (void) \
{ return node_special_func_new(SPECIAL_ ## FUNC); }

DEFINE_SPECIAL_MAKER(IF)
DEFINE_SPECIAL_MAKER(LAMBDA)
DEFINE_SPECIAL_MAKER(MK_CONT)
DEFINE_SPECIAL_MAKER(QUOTE)
DEFINE_SPECIAL_MAKER(DEF)
DEFINE_SPECIAL_MAKER(SET)

#define ARR_LEN(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

foreign_assoc_t startenv[] = {
	{ "quote",    make_special_QUOTE },
	{ "if",       make_special_IF },
	{ "lambda",   make_special_LAMBDA },
	{ "call/cc",  make_special_MK_CONT },
	{ "def!",     make_special_DEF },
	{ "set!",     make_special_SET },

	{ "nil?",     make_foreign_nil_p },
	{ "value?",   make_foreign_value_p },
	{ "sym?",     make_foreign_sym_p },
	{ "cons?",    make_foreign_cons_p },
	{ "func?",    make_foreign_func_p },

	{ "car",      make_foreign_car },
	{ "cdr",      make_foreign_cdr },
	{ "makesym",  make_foreign_makesym },
	{ "splitsym", make_foreign_splitsym },
	{ "cons",     make_foreign_cons },

	{ "symeq?",   make_foreign_symeq_p },
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

	{ "b_and",    make_foreign_bit_and },
	{ "b_or",     make_foreign_bit_or },
	{ "b_nand",   make_foreign_bit_nand },
	{ "b_xor",    make_foreign_bit_xor },
	{ "b_shl",    make_foreign_bit_shl },
	{ "b_shr",    make_foreign_bit_shr },
	{ "b_shra",   make_foreign_bit_shra },


};

int main(int argc, char *argv[])
{
	node_t *parse_result, *eval_result, *env_handle = NULL;
	eval_err_t eval_stat;
	parse_err_t parse_stat;
	char *remain;
	size_t i;

	nodes_initialize();

	/* setup environment */
	printf("*** initialize environment ***\n");
	env_handle = node_handle_new(NULL);
	node_lockroot(env_handle);
	{
		node_t *key, *value;
		for (i = 0; i < ARR_LEN(startenv); i++) {
			key = node_symbol_new(startenv[i].name);
			value = startenv[i].func();

			environ_add(env_handle, key, value);
		}
	}

	/* parse + eval argv */
	for(i = 1; i < (unsigned) argc; i++) {
		printf("*** parse %s ***\n", argv[i]);
		parse_stat = parse(argv[i], &remain, &parse_result);
		if(parse_stat != PARSE_OK) {
			printf("parse error for: %s\n", remain);
			printf("-- %s\n", parse_err_str(parse_stat));
			continue;
		}
		node_lockroot(parse_result); /* required before passing to eval() */

		printf("parse result: %p\n", parse_result);
		node_print_pretty(parse_result, false);
		printf("\n");

		printf("start environment:\n");
		environ_print(node_handle(env_handle));

		printf("*** eval ***\n");
		eval_stat = eval(parse_result, env_handle, &eval_result);

		if(eval_stat) {
			printf("eval error for: \n");
			node_print_pretty(eval_result, false);
			printf("\n");
			printf("%s\n", eval_err_str(eval_stat));
			return -1;
		}
		printf("eval result: %p\n", eval_result);
		node_print_pretty(eval_result, false);
		printf("\n");
		//node_print_recursive(eval_result);

		
		//printf("*** gc state following eval ***\n");
		//node_gc_state();

		printf("*** releasing parse result %p ***\n", parse_result);
		node_droproot(parse_result);

		printf("*** releasing eval result %p ***\n", eval_result);
		node_droproot(eval_result);
	}

	printf("*** result environment %p ***\n", node_handle(env_handle));
	environ_print(node_handle(env_handle));

	printf("*** release toplevel environment %p ***\n", env_handle);
	node_droproot(env_handle);

	
	printf("*** cleanup ***\n");

	node_gc_print_state();

	printf("*** run gc ***\n");
	node_gc();
	printf("*** run gc ***\n");
	node_gc();

	printf("*** final state ***\n");

	node_gc_print_state();

	return 0;
}
