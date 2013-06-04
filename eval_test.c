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

DEFINE_FOREIGN_MAKER(foreign_quote)
DEFINE_FOREIGN_MAKER(foreign_defbang)
DEFINE_FOREIGN_MAKER(foreign_setbang)
DEFINE_FOREIGN_MAKER(foreign_atom)
DEFINE_FOREIGN_MAKER(foreign_car)
DEFINE_FOREIGN_MAKER(foreign_cdr)

node_t *make_foreign_if(void)
{
	return node_if_func_new();
}

DEFINE_FOREIGN_MAKER(foreign_cons)
DEFINE_FOREIGN_MAKER(foreign_eq)

node_t *make_foreign_lambda(void)
{
	return node_lambda_func_new();
}

DEFINE_FOREIGN_MAKER(foreign_makesym)
DEFINE_FOREIGN_MAKER(foreign_splitsym)

#define ARR_LEN(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

foreign_assoc_t foreigns[] = {
	{ "quote",    make_foreign_quote },
	{ "def!",     make_foreign_defbang },
	{ "set!",     make_foreign_setbang },
	{ "atom",     make_foreign_atom },
	{ "car",      make_foreign_car },
	{ "cdr",      make_foreign_cdr },
	{ "if",       make_foreign_if },
	{ "cons",     make_foreign_cons },
	{ "eq",       make_foreign_eq },
	{ "lambda",   make_foreign_lambda },
	{ "makesym",  make_foreign_makesym },
	{ "splitsym", make_foreign_splitsym },
};

int main(int argc, char *argv[])
{
	node_t *parse_result, *eval_result, *env = NULL;
	eval_err_t eval_stat;
	parse_err_t parse_stat;
	char *remain;
	size_t i;

	nodes_initialize();

	/* setup environment */
	printf("*** initialize environment ***\n");
	{
		node_t *key, *value;
		for (i = 0; i < ARR_LEN(foreigns); i++) {
			key = node_symbol_new(foreigns[i].name);
			value = foreigns[i].func();

			environ_add(&env, key, value);
		}
	}
	assert(! env || node_isroot(env));

	/* parse + eval argv */
	for(i = 1; i < (unsigned) argc; i++) {
		assert(! env || node_isroot(env));

		printf("*** parse %s ***\n", argv[i]);
		parse_stat = parse(argv[i], &remain, &parse_result);
		if(parse_stat != PARSE_OK) {
			printf("parse error for: %s\n", remain);
			printf("-- %s\n", parse_err_str(parse_stat));
			continue;
		}
		node_lockroot(parse_result); /* required before passing to eval() */

		assert(! env || node_isroot(env));

		printf("parse result: %p\n", parse_result);
		node_print_pretty(parse_result, false);
		printf("\n");

		assert(! env || node_isroot(env));

		printf("start environment:\n");
		environ_print(env);

		printf("*** eval ***\n");
		//eval_stat = eval(parse_result, &env, &eval_result);
		eval_stat = eval_norec(parse_result, &env, &eval_result);

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

		assert(! env || node_isroot(env));

		printf("*** releasing parse result %p ***\n", parse_result);
		node_droproot(parse_result);

		printf("*** releasing eval result %p ***\n", eval_result);
		node_droproot(eval_result);

		assert(! env || node_isroot(env));
	}

	printf("*** result environment %p ***\n", env);
	environ_print(env);

	printf("*** release toplevel environment %p ***\n", env);
	node_droproot(env);

	
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
