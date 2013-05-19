#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "environ.h"
#include "builtins.h"
#include "eval.h"

eval_err_t foreign_quote(node_t *args, node_t **env, node_t **result)
{
	(void) env;
	*result = args;

	if(node_type(args) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	if(node_cons_cdr(args)) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	*result = node_cons_car(args);

	return EVAL_OK;
}

static bool check_atom(node_t* arg)
{
	return (!arg || arg->type == NODE_VALUE || arg->type == NODE_SYMBOL);
}

eval_err_t foreign_atom(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	if(node_cons_cdr(args)) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	/* eval arg */
	status = eval(node_cons_car(args), env, &temp);
	if(status != EVAL_OK) {
		*result = temp;
		goto finish;
	}
	
	/* temp must be NULL, a value, or a symbol to be an atom */
	if(check_atom(temp)) {
		*result = node_value_new(1);
	} else {
		*result = NULL;
	}

finish:
	node_droproot(temp);
	return status;
}

eval_err_t foreign_car(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	if(node_cons_cdr(args)) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	/* evai arg */
	status = eval(node_cons_car(args), env, &temp);
	if(status != EVAL_OK) {
		goto finish;
	}

	/* temp must be a cons */
	if(node_type(temp) != NODE_CONS) {
		status = EVAL_ERR_EXPECTED_CONS;
		goto finish;
	}
	
	*result = node_cons_car(temp);

finish:
	node_droproot(temp);
	return status;
}

eval_err_t foreign_cdr(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	if(node_cons_cdr(args)) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	/* evai arg */
	status = eval(node_cons_car(args), env, &temp);
	if(status != EVAL_OK) {
		*result = temp;
		goto finish;
	}

	/* temp must be a cons */
	if(node_type(temp) != NODE_CONS) {
		status = EVAL_ERR_EXPECTED_CONS;
		*result = temp;
		goto finish;
	}
	
	*result = node_cons_cdr(temp);

finish:
	node_droproot(temp);
	return status;
}

eval_err_t foreign_cons(node_t *args, node_t **env, node_t **result)
{
	node_t *child = NULL, *next = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	if(!node_cons_cdr(args)) {
		return EVAL_ERR_MISSING_ARG;
	}

	if(node_type(node_cons_cdr(args)) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	if(node_cons_cdr(node_cons_cdr(args))) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	/* eval args */
	status = eval(node_cons_car(args), env, &child);
	if(status != EVAL_OK) {
		*result = child;
		goto finish;
	}

	status = eval(node_cons_car(node_cons_cdr(args)), env, &next);
	if(status != EVAL_OK) {
		*result = next;
		goto finish;
	}

	*result = node_cons_new(child, next);

finish:
	return status;
}

eval_err_t foreign_eq(node_t *args, node_t **env, node_t **result)
{
	node_t *temp = NULL, *temp2 = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	if(! node_cons_cdr(args)) {
		return EVAL_ERR_MISSING_ARG;
	}

	if(node_type(node_cons_cdr(args)) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	if(node_cons_cdr(node_cons_cdr(args))) {
		return EVAL_ERR_TOO_MANY_ARGS;
	}

	/* eval args */
	status = eval(node_cons_car(args), env, &temp);
	if(status != EVAL_OK) {
		*result = temp;
		goto finish;
	}
	status = eval(node_cons_car(node_cons_cdr(args)), env, &temp2);
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
	    strcmp(node_symbol_name(temp), node_symbol_name(temp2)))) {
		*result = NULL;
	} else {
		*result = node_value_new(1);
	}

finish:
	node_droproot(temp);
	node_droproot(temp2);
	return status;
}

eval_err_t foreign_defbang(node_t *args, node_t **env, node_t **result)
{
	node_t *val, *newval, *name;
	eval_err_t status = EVAL_OK;
	*result = args;

	if(node_type(args) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	/* note: we return the symbol that was added to environment */
	*result = name = node_cons_car(args);
	if(node_type(name) != NODE_SYMBOL) {
		return EVAL_ERR_EXPECTED_SYMBOL;
	}

	/* eval passed value */
	if(node_type(node_cons_cdr(args)) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}
	val = node_cons_car(node_cons_cdr(args));

	status = eval(val, env, &newval);
	if(status != EVAL_OK) {
		return status;
	}

	environ_add(env, name, newval);

	return status;
}

eval_err_t foreign_setbang(node_t *args, node_t **env, node_t **result)
{
	node_t *name = NULL, *val = NULL, *newval = NULL, *keyval = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_type(args) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}

	name = node_cons_car(args);
	if(node_type(name) != NODE_SYMBOL) {
		return EVAL_ERR_EXPECTED_SYMBOL;
	}

	if(node_type(node_cons_cdr(args)) != NODE_CONS) {
		return EVAL_ERR_EXPECTED_CONS;
	}
	val = node_cons_car(node_cons_cdr(args));

	status = eval(val, env, &newval);
	if(status != EVAL_OK) {
		return status;
	}

	if(! environ_keyvalue(*env, name, &keyval)) {
		node_droproot(newval);
		status = EVAL_ERR_UNRESOLVED_SYMBOL;
	}

	node_cons_patch_cdr(keyval, newval);
	*result = newval;

	return EVAL_OK;
}
