#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "environ.h"
#include "builtins.h"
#include "eval.h"

eval_err_t builtin_quote(node_t *args, node_t **env, node_t **result)
{
	(void) env;
	*result = args;

	if(node_type(args) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	if(node_next_noref(args)) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	*result = node_child_noref(args);

	return EVAL_OK;
}

static bool check_atom(node_t* arg)
{
	return (!arg || arg->type == NODE_VALUE || arg->type == NODE_SYMBOL);
}

eval_err_t builtin_atom(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	if(node_next_noref(args)) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	/* eval arg */
	status = eval(node_child_noref(args), env, &temp);
	if(status != EVAL_OK) {
		*result = temp;
		goto finish;
	}
	
	/* temp must be NULL, a value, or a symbol to be an atom */
	if(check_atom(temp)) {
		*result = node_new_value(1);
	} else {
		*result = NULL;
	}

finish:
	node_forget(temp);
	return status;
}

eval_err_t builtin_car(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	if(node_next_noref(args)) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	/* evai arg */
	status = eval(node_child_noref(args), env, &temp);
	if(status != EVAL_OK) {
		goto finish;
	}

	/* temp must be a list */
	if(node_type(temp) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}
	
	*result = node_child_noref(temp);

finish:
	node_forget(temp);
	return status;
}

eval_err_t builtin_cdr(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	if(node_next_noref(args)) {
		return EVAL_ERR_TOO_MANY_ARGS;
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
	
	*result = node_next_noref(temp);

finish:
	node_forget(temp);
	return status;
}

eval_err_t builtin_cons(node_t *args, node_t **env, node_t **result)
{
	node_t *child = NULL, *next = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	if(!node_next_noref(args)) {
		return EVAL_ERR_MISSING_ARG;
	}

	if(node_type(node_next_noref(args)) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	if(node_next_noref(node_next_noref(args))) {
		return EVAL_ERR_TOO_MANY_ARGS;
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
	return status;
}

eval_err_t builtin_eq(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL, *temp2 = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	if(! node_next_noref(args)) {
		return EVAL_ERR_MISSING_ARG;
	}

	if(node_type(node_next_noref(args)) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	if(node_next_noref(node_next_noref(args))) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	/* eval args */
	status = eval(node_child_noref(args), env, &temp);
	if(status != EVAL_OK) {
		*result = temp;
		goto finish;
	}
	status = eval(node_child_noref(node_next_noref(args)), env, &temp2);
	if(status != EVAL_OK) {
		*result = temp2;
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
	node_forget(temp);
	node_forget(temp2);
	return status;
}

eval_err_t builtin_defbang(node_t *args, node_t **env, node_t **result)
{
	*result = args;

	if(node_type(args) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	/* note: we return the symbol that was added to environment */
	*result = node_child_noref(args);
	if(node_type(*result) != NODE_SYMBOL) {
		return EVAL_ERR_EXPECTED_SYMBOL;
	}

	environ_add(env, *result, NULL);

	return EVAL_OK;
}

eval_err_t builtin_setbang(node_t *args, node_t **env, node_t **result)
{
	node_t *name = NULL, *val = NULL, *newval = NULL, *keyval = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}

	name = node_child_noref(args);
	if(node_type(name) != NODE_SYMBOL) {
		return EVAL_ERR_EXPECTED_SYMBOL;
	}

	if(node_type(node_next_noref(args)) != NODE_LIST) {
		return EVAL_ERR_EXPECTED_LIST;
	}
	val = node_child_noref(node_next_noref(args));

	status = eval(val, env, &newval);
	if(status != EVAL_OK) {
		return status;
	}

	if(! environ_keyvalue(*env, name, &keyval)) {
		node_forget(newval);
		status = EVAL_ERR_UNRESOLVED_SYMBOL;
	}

	node_patch_list_next(keyval, newval);
	*result = newval;

	return EVAL_OK;
}
