#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "environ.h"
#include "builtins.h"
#include "eval.h"

static bool check_atom(node_t* arg)
{
	return (!arg || node_type(arg) == NODE_VALUE || node_type(arg) == NODE_SYMBOL);
}

eval_err_t foreign_atom(node_t *args, node_t **result)
{
	node_t *temp;
	eval_err_t status = EVAL_OK;

	*result = args;

	temp = node_cons_car(args);
	if(node_cons_cdr(args)) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	/* arg must be NULL, a value, or a symbol to be an atom */
	if(check_atom(temp)) {
		*result = node_value_new(1);
	} else {
		*result = NULL;
	}

finish:
	return status;
}

eval_err_t foreign_car(node_t *args, node_t **result)
{
	node_t *temp;
	eval_err_t status = EVAL_OK;

	*result = args;

	temp = node_cons_car(args);
	if(node_cons_cdr(args)) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	if(node_type(temp) != NODE_CONS) {
		status = EVAL_ERR_EXPECTED_CONS;
		goto finish;
	}
	
	*result = node_cons_car(temp);

finish:
	return status;
}

eval_err_t foreign_cdr(node_t *args, node_t **result)
{
	node_t *temp;
	eval_err_t status = EVAL_OK;

	*result = args;

	temp = node_cons_car(args);
	if(node_cons_cdr(args)) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	if(node_type(temp) != NODE_CONS) {
		status = EVAL_ERR_EXPECTED_CONS;
		goto finish;
	}
	
	*result = node_cons_cdr(temp);

finish:
	return status;
}

eval_err_t foreign_cons(node_t *args, node_t **result)
{
	node_t *child = NULL, *next = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	child = node_cons_car(args);
	if(!node_cons_cdr(args)) {
		status = EVAL_ERR_MISSING_ARG;
		goto finish;
	}
	next = node_cons_car(node_cons_cdr(args));
	if(node_cons_cdr(node_cons_cdr(args))) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	*result = node_cons_new(child, next);

finish:
	return status;
}

eval_err_t foreign_eq(node_t *args, node_t **result)
{
	node_t *temp = NULL, *temp2 = NULL;
	eval_err_t status = EVAL_OK;

	*result = args;

	temp = node_cons_car(args);
	if(!node_cons_cdr(args)) {
		status = EVAL_ERR_MISSING_ARG;
		goto finish;
	}
	temp2 = node_cons_car(node_cons_cdr(args));
	if(node_cons_cdr(node_cons_cdr(args))) {
		status = EVAL_ERR_TOO_MANY_ARGS;
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
	return status;
}

eval_err_t foreign_makesym(node_t *args, node_t **result)
{
	char name[MAX_SYM_LEN], *cursor;;
	node_t *val, *val_iter;
	eval_err_t status = EVAL_OK;

	*result = args;

	if(node_cons_cdr(args)) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	val = node_cons_car(args);
	if(node_type(val) != NODE_CONS) {
		status = EVAL_ERR_EXPECTED_CONS;
		goto finish;
	}
	
	val_iter = val;
	cursor = name;
	for(val_iter = val; val_iter; val_iter = node_cons_cdr(val_iter)) {
		val = node_cons_car(val_iter);
		if(node_type(val) != NODE_VALUE) {
			return EVAL_ERR_EXPECTED_VALUE;
		}
		if(node_value(val) > 255 ) {
			return EVAL_ERR_VALUE_BOUNDS;
		}
		*cursor++ = node_value(val);
		if(cursor - &(name[0]) >= (ssize_t) sizeof(name)) {
			break;
		}
		val_iter = node_cons_cdr(val_iter);
	}
	*cursor = 0;
	*result = node_symbol_new(name);

finish:
	return status;
}

eval_err_t foreign_splitsym(node_t *args, node_t **result)
{
	char *start, *end;
	node_t *sym;
	eval_err_t status = EVAL_OK;

	*result = args;
	sym = node_cons_car(args);
	if(node_cons_cdr(args)) {
		status = EVAL_ERR_TOO_MANY_ARGS;
		goto finish;
	}

	if(node_type(sym) != NODE_SYMBOL) {
		status = EVAL_ERR_EXPECTED_SYMBOL;
		goto finish;
	}

	start = node_symbol_name(sym);
	end = start + strlen(start) - 1; // end points to last nonzero char

	/* construct value list backwards */
	for(*result = NULL; end >= start; end--) {
		*result = node_cons_new(node_value_new(*end), *result);
	}

finish:
	return EVAL_OK;
}
