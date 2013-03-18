#include <stdlib.h>
#include <stdio.h>

#include "parse.h"
#include "eval.h"
#include "environ_utils.h"
#include "builtins.h"

builtin_assoc_t builtins[] = {
	{ "atom",  builtin_atom },
	{ "car",   builtin_car },
	{ "cdr",   builtin_cdr },
	{ "cond",  builtin_cond },
	{ "cons",  builtin_cons },
	{ "eq",    builtin_eq },
	{ "quote", builtin_quote },
	{ "label", builtin_label }
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
	env = environ_add_builtins(env, builtins, 
	                           sizeof(builtins) / sizeof(builtins[0]));
	printf("start environment:\n");
	environ_print(env);

	printf("live nodes:\n");
	node_find_live(print_callback, NULL);
	printf("dead nodes:\n");
	node_find_free(print_callback, NULL);

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

		printf("*** eval %s ***\n", argv[i]);
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

		node_release(parse_result);
		node_release(eval_result);
	}

	node_release(env);

	printf("*** cleanup ***\n");
	count = node_gc();
	printf("%llu nodes freed\n", (unsigned long long) count);

	printf("leaked nodes:\n");
	node_find_live(print_callback, NULL);

	return 0;
}
