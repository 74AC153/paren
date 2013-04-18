#include <stdlib.h>
#include <stdio.h>

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

int print_callback(node_t *n, void *p)
{
	p = p;
	node_print(n, false);
	return 0;
}

int main(int argc, char *argv[])
{
	node_t *parse_result, *eval_result, *env = NULL;
	eval_err_t eval_stat;
	parse_err_t parse_stat;
	char *remain;
	int i;
	size_t count;

	if(argc < 2) {
		printf("usage: %s 'string'\n", argv[0]);
	}

	/* setup environment */
	printf("*** initialize environment ***\n");
	env = environ_add_builtins(env, builtins, 
	                           sizeof(builtins) / sizeof(builtins[0]));
	printf("start environment:\n");
	environ_print(env);

	printf("live nodes:\n");
	node_find_live(print_callback, NULL);
	printf("dead nodes:\n");
	node_find_free(print_callback, NULL);
	node_sanity();

	for(i = 1; i < argc; i++) {
		printf("*** parse %s ***\n", argv[i]);
		parse_stat = parse(argv[i], &remain, &parse_result);
		if(parse_stat != PARSE_OK) {
			printf("parse error for: %s\n", remain);
			printf("-- %s\n", parse_err_str(parse_stat));
			continue;
		}

		printf("parse result:\n");
		node_print_pretty(parse_result);
		printf("\n");

		printf("remaining: %s\n", remain);
		printf("live nodes:\n");
		node_find_live(print_callback, NULL);
		node_sanity();

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

		printf("result environment:\n");
		environ_print(env);

		printf("live nodes:\n");
		node_find_live(print_callback, NULL);

		node_sanity();

		printf("*** releasing parse result ***\n");
		node_release(parse_result);

		printf("*** releasing eval result ***\n");
		node_release(eval_result);

		node_sanity();
	}

	printf("*** release toplevel environment ***\n");
	node_release(env);

	printf("*** cleanup ***\n");
	count = node_gc();
	printf("%llu nodes freed\n", (unsigned long long) count);

	printf("leaked nodes:\n");
	node_find_live(print_callback, NULL);

	return 0;
}
