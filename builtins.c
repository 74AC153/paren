#include <stdlib.h>
#include <string.h>

#include "environ.h"
#include "builtins.h"
#include "eval.h"

static bool check_atom(node_t* arg)
{
	return (!arg || arg->type == NODE_VALUE || arg->type == NODE_SYMBOL);
}

int builtin_atom(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}

	if(node_next_noref(args)) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	/* evai arg */
	status = eval(node_child_noref(args), env, &temp);
	if(status != EVAL_OK) {
		goto finish;
	}
	
	/* temp must be NULL, a value, or a symbol to be an atom */
	if(check_atom(temp)) {
		*result = node_new_value(1);
	} else {
		*result = NULL;
	}

finish:
	node_release(temp);
	return status;
}

int builtin_car(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}

	if(node_next_noref(args)) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	/* evai arg */
	status = eval(node_child_noref(args), env, &temp);
	if(status != EVAL_OK) {
		goto finish;
	}

	/* temp must be a list */
	if(node_type(temp) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		*result = temp;
		goto finish;
	}
	
	*result = node_child(temp);

finish:
	node_release(temp);
	return status;
}

int builtin_cdr(node_t *args, node_t **env, node_t **result)
{
	node_t *temp;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}

	if(node_next_noref(args)) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	/* evai arg */
	status = eval(node_child_noref(args), env, &temp);
	if(status != EVAL_OK) {
		*result = temp;
		goto finish;
	}

	/* temp must be a list */
	if(node_type(temp) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		*result = temp;
		goto finish;
	}
	
	*result = node_next(temp);

finish:
	node_release(temp);
	return status;
}

/*
	(cond ((test) (rest)) ((test2) (rest2)) ...)

	args
    +---+---+
    |   |  ---> ...
    +-|-+---+
      |
      v
    pair-test   pair-rest
    +---+---+   +---+---+
    |   |  ---> |   |nil|
    +-|-+---+   +-|-+---+
      |           |
      v           v
    test        rest
*/

int builtin_cond(node_t *args, node_t **env, node_t **result)
{
	node_t *args_curs, *pair_test, *test, *pair_rest, *rest, *test_res;
	eval_err_t status = EVAL_OK;

	for(args_curs = args; args_curs; args_curs = node_next_noref(args_curs)) {
		pair_test = node_child_noref(args_curs);
		pair_rest = node_next_noref(pair_test);
		test = node_child_noref(pair_test);
		rest = node_child_noref(pair_rest);
		status = eval(test, env, &test_res);
		if(status != EVAL_OK) {
			*result = test_res;
			goto finish;
		}
		if(test_res) {
			node_release(test_res);
			status = eval(rest, env, result);
			break;
		}
	}
finish:
	return status;
}

int builtin_cons(node_t *args, node_t **env, node_t **result)
{
	node_t *child = NULL, *next = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}

	if(!node_next_noref(args)) {
		status = EVAL_ERR_MISSING_ARG;
		goto finish;
	}

	if(node_type(node_next_noref(args)) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}

	if(node_next_noref(node_next_noref(args))) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	/* eval args */
	status = eval(node_child_noref(args), env, &child);
	if(status != EVAL_OK) {
		*result = child;
		goto finish;
	}

	status = eval(node_child_noref(node_next_noref(args)), env, &next);
	if(status != EVAL_OK) {
		*result = next;
		goto finish;
	}

	*result = node_new_list(child, next);

finish:
	node_release(child);
	node_release(next);
	return status;
}

int builtin_eq(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL, *temp2 = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}

	if(! node_next_noref(args)) {
		status = EVAL_ERR_MISSING_ARG;
		goto finish;
	}

	if(node_type(node_next_noref(args)) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}

	if(node_next_noref(node_next_noref(args))) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	/* evai args */
	status = eval(node_child_noref(args), env, &temp);
	if(status != EVAL_OK) {
		goto finish;
	}
	status = eval(node_child_noref(node_next_noref(args)), env, &temp2);
	if(status != EVAL_OK) {
		goto finish;
	}
	
	if(!check_atom(temp) ||
	   !check_atom(temp2) ||
	   node_type(temp) != node_type(temp2) ||
	   (node_type(temp) == NODE_VALUE &&
	    node_value(temp) != node_value(temp2)) ||
	   (node_type(temp) == NODE_SYMBOL && node_type(temp2) == NODE_SYMBOL &&
	    strcmp(node_name(temp), node_name(temp2)))) {
		*result = NULL;
	} else {
		*result = node_new_value(1);
	}

finish:
	node_release(temp);
	node_release(temp2);
	return status;
}

int builtin_quote(node_t *args, node_t **env, node_t **result)
{
	eval_err_t status = EVAL_OK;

	env = env;
	*result = args;

	if(node_type(args) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}

	if(node_next_noref(args)) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	*result = node_child(args);

finish:
	return EVAL_OK;
}

int builtin_label(node_t *args, node_t **env, node_t **result)
{
	node_t *key = NULL, *valexpr = NULL, *val = NULL, *oldenv = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}
	if(!node_child_noref(args)) {
		status = EVAL_ERR_MISSING_ARG;
		goto finish;
	}
	if(node_type(node_child_noref(args)) != NODE_SYMBOL) {
		status = EVAL_ERR_EXPECTED_SYMBOL;
		goto finish;
	}
	if(!node_next_noref(args)) {
		status = EVAL_ERR_MISSING_ARG;
		goto finish;
	}
	if(node_type(node_next_noref(args)) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}
	if(node_next_noref(node_next_noref(args)) != NULL) {
		status = EVAL_ERR_MISSING_ARG;
		goto finish;
	}

	key = node_child(args);
	if(node_type(key) != NODE_SYMBOL) {
		status = EVAL_ERR_EXPECTED_SYMBOL;
		goto finish;
	}

	valexpr = node_child(node_next_noref(args));

	status = eval(valexpr, env, &val);
	if(status != EVAL_OK) {
		goto finish;
	}

	oldenv = *env;
	*env = environ_add(oldenv, key, val);

	*result = node_retain(val);

finish:
	node_release(key);
	node_release(valexpr);
	node_release(val);
	node_release(oldenv);
	return status;
}
