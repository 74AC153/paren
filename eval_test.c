#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "parse.h"
#include "eval.h"
#include "environ_utils.h"
#include "builtins.h"

#define DEFINE_FOREIGN_MAKER(FUNC) \
static node_t *make_ ## FUNC (void) \
{ \
	return node_foreign_new(FUNC); \
}

DEFINE_FOREIGN_MAKER(foreign_atom)
DEFINE_FOREIGN_MAKER(foreign_car)
DEFINE_FOREIGN_MAKER(foreign_cdr)
DEFINE_FOREIGN_MAKER(foreign_cons)
DEFINE_FOREIGN_MAKER(foreign_eq)
DEFINE_FOREIGN_MAKER(foreign_makesym)
DEFINE_FOREIGN_MAKER(foreign_splitsym)

node_t *make_special_if(void)
{
	return node_special_func_new(SPECIAL_IF);
}
node_t *make_special_lambda(void)
{
	return node_special_func_new(SPECIAL_LAMBDA);
}
node_t *make_special_cont(void)
{
	return node_special_func_new(SPECIAL_MK_CONT);
}
node_t *make_special_quote(void)
{
	return node_special_func_new(SPECIAL_QUOTE);
}
node_t *make_special_defbang(void)
{
	return node_special_func_new(SPECIAL_DEF);
}
node_t *make_special_setbang(void)
{
	return node_special_func_new(SPECIAL_SET);
}

#define ARR_LEN(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

foreign_assoc_t foreigns[] = {
	{ "quote",    make_special_quote },
	{ "if",       make_special_if },
	{ "lambda",   make_special_lambda },
	{ "call/cc",  make_special_cont },
	{ "def!",     make_special_defbang },
	{ "set!",     make_special_setbang },

	{ "atom",     make_foreign_atom },
	{ "car",      make_foreign_car },
	{ "cdr",      make_foreign_cdr },
	{ "cons",     make_foreign_cons },
	{ "eq",       make_foreign_eq },
	{ "makesym",  make_foreign_makesym },
	{ "splitsym", make_foreign_splitsym },
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
		for (i = 0; i < ARR_LEN(foreigns); i++) {
			key = node_symbol_new(foreigns[i].name);
			value = foreigns[i].func();

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
		eval_stat = eval_norec(parse_result, env_handle, &eval_result);

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
