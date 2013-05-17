#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "parse.h"
#include "eval.h"
#include "environ_utils.h"
#include "builtins.h"

#define DEFINE_BUILTIN_MAKER(FUNC) \
node_t *make_ ## FUNC (void) \
{ \
	return node_new_builtin(FUNC); \
}

DEFINE_BUILTIN_MAKER(builtin_quote)
DEFINE_BUILTIN_MAKER(builtin_defbang)
DEFINE_BUILTIN_MAKER(builtin_setbang)
DEFINE_BUILTIN_MAKER(builtin_atom)
DEFINE_BUILTIN_MAKER(builtin_car)
DEFINE_BUILTIN_MAKER(builtin_cdr)

node_t *make_builtin_if(void)
{
	return node_new_if_func();
}

DEFINE_BUILTIN_MAKER(builtin_cons)
DEFINE_BUILTIN_MAKER(builtin_eq)

node_t *make_builtin_lambda(void)
{
	return node_new_lambda_func();
}

#define ARR_LEN(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

/* rewrite these... */
builtin_assoc_t builtins[] = {
	{ "quote",  make_builtin_quote },
	{ "def!",   make_builtin_defbang },
	{ "set!",   make_builtin_setbang },
	{ "atom",   make_builtin_atom },
	{ "car",    make_builtin_car },
	{ "cdr",    make_builtin_cdr },
	{ "if",     make_builtin_if },
	{ "cons",   make_builtin_cons },
	{ "eq",     make_builtin_eq },
	{ "lambda", make_builtin_lambda },
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
		for (i = 0; i < ARR_LEN(builtins); i++) {
			key = node_new_symbol(builtins[i].name);
			value = builtins[i].func();

			environ_add(&env, key, value);
		}
	}
	assert(! env || node_is_remembered(env));

	/* parse + eval argv */
	for(i = 1; i < (unsigned) argc; i++) {
		assert(! env || node_is_remembered(env));

		printf("*** parse %s ***\n", argv[i]);
		parse_stat = parse(argv[i], &remain, &parse_result);
		if(parse_stat != PARSE_OK) {
			printf("parse error for: %s\n", remain);
			printf("-- %s\n", parse_err_str(parse_stat));
			continue;
		}

		assert(! env || node_is_remembered(env));

		printf("parse result:\n");
		node_print_pretty(parse_result);
		printf("\n");

		assert(! env || node_is_remembered(env));

		printf("start environment:\n");
		environ_print(env);

		printf("*** eval ***\n");
		eval_stat = eval(parse_result, &env, &eval_result);

		if(eval_stat) {
			printf("eval error for: \n");
			node_print_pretty(eval_result);
			printf("\n");
			printf("%s\n", eval_err_str(eval_stat));
			return -1;
		}
		printf("eval result:\n");
		node_print_pretty(eval_result);
		printf("\n");
		//node_print_recursive(eval_result);

		
		//printf("*** gc state following eval ***\n");
		//node_gc_state();

		assert(! env || node_is_remembered(env));

		printf("*** releasing parse result %p ***\n", parse_result);
		node_forget(parse_result);

		printf("*** releasing eval result %p ***\n", eval_result);
		node_forget(eval_result);

		assert(! env || node_is_remembered(env));
	}

	printf("*** result environment %p ***\n", env);
	environ_print(env);

	printf("*** release toplevel environment %p ***\n", env);
	node_forget(env);

	
	printf("*** cleanup ***\n");

	node_gc_state();

	printf("*** run gc ***\n");
	node_gc();
	printf("*** run gc ***\n");
	node_gc();

	printf("*** final state ***\n");

	node_gc_state();

	return 0;
}
