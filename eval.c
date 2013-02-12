#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "environ.h"
#include "eval.h"

/*
(lamba (vars) (expr))

func        vars_node   expr_node
+---+---+   +---+---+   +---+---+
|   |  ---> |   |  ---> |   |nil|
+-|-+---+   +-|-+---+   +-|-+---+
  |           |           | expr +---+---+
  v           v           +----> |   |   |
+--------+   vars                +---+---+
| symbol |   +---+---+   +---+---+
+-|------+   |   |  ---> |   |   |
  |          +-|-+---+   +---+---+
  v            |
 "lambda"      v
             +--------+
             | symbol |
             +--------+
*/

static bool traverse(node_t *n, char *dir, node_t **result)
{
	while(dir && *dir) {
		if(node_type(n) != NODE_LIST) {
			return false;
		}
		if(*dir == 'c') {
			n = node_child_noref(n);
		} else {
			n = node_next_noref(n);
		}
		dir++;
	}
	if(result) {
		*result = node_retain(n);
	}
	return n != NULL;
}

eval_err_t validate_lambda_form(node_t *input)
{
	eval_err_t status = EVAL_OK;
	node_t *sym, *vars_list, *var;

	/* ensure input is of form (lambda (...) ...) */

	sym = node_child_noref(input);
	if(node_type(sym) != NODE_SYMBOL) {
		status = EVAL_ERR_EXPECTED_SYMBOL;
		goto finish;
	}

	if(strcmp(node_name(sym), "lambda")) {
		status = EVAL_ERR_FUNC_IS_NOT_LAMBDA;
		goto finish;
	}

	if(node_type(node_next_noref(input)) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}
	vars_list = node_child_noref(node_next_noref(input));
	if(node_type(vars_list) != NODE_LIST) {
		status = EVAL_ERR_EXPECTED_LIST;
		goto finish;
	}
	for( ; vars_list; vars_list = node_next_noref(vars_list)) {
		var = node_child_noref(vars_list);
		if(node_type(var) != NODE_SYMBOL && node_type(var) != NODE_LIST) {
			status = EVAL_ERR_EXPECTED_LIST_SYM;
			goto finish;
		}
	}
	
finish:	
	return status;
}

eval_err_t lambda_call(
	node_t *vars,
	node_t *args,
	node_t *expr,
	node_t **environ,
	node_t **result)
{
	node_t *var, *arg, *argeval, *newenv, *oldenv;
	eval_err_t status = EVAL_OK;

	newenv = environ_push(*environ);

	/* create a new environment with the specified bindings */
	for(;
	    vars && args;
	    vars = node_next_noref(vars), args = node_next_noref(args)) {

		if(node_type(vars) != NODE_LIST) {
			status = EVAL_ERR_EXPECTED_LIST;
			*result = vars;
			goto finish;
		}
		var = node_child_noref(vars);

		if(node_type(args) != NODE_LIST) {
			status = EVAL_ERR_EXPECTED_LIST;
			*result = args;
			goto finish;
		}
		arg = node_child_noref(args);

		if(node_type(var) != NODE_SYMBOL) {
			status = EVAL_ERR_EXPECTED_SYMBOL;
			*result = var;
			goto finish;
		}

		status = eval(arg, environ, &argeval);
		if(status != EVAL_OK) {
			*result = arg;
			goto finish;
		}

		oldenv = newenv;
		newenv = environ_add(oldenv, var, argeval);
		node_release(oldenv);
		node_release(argeval);
	}
	/* eval expr in new environment */
	status = eval(expr, &newenv, result);

finish:
	node_release(newenv);
	return status;
}

eval_err_t eval(node_t *input, node_t **environ, node_t **output)
{
	eval_err_t status = EVAL_OK;
	bool lookup_stat = false;
	node_t *func = NULL, *args = NULL;
	node_t *sym = NULL, *vars = NULL, *expr = NULL;

	/* not forcing this necessitates messy refcounting logic */
	assert(!input || *output != input);

	switch(node_type(input)) {
	case NODE_NIL:
		*output = NULL;
		break;

	case NODE_DEAD:
		assert(node_type(input) != NODE_DEAD);
		break;

	case NODE_LIST:
		/*
		   (lambda (...) ...) ?
		*/
		status = validate_lambda_form(input);
		if(status == EVAL_OK) {
			*output = node_retain(input);
			break;
		}

		/*
		   funcall:
		   ((lambda ...) args ...)
		   (symbol args ...)
		*/
		args = node_next(input);
		func = node_child(input);

		if(node_type(func) == NODE_SYMBOL) {
			node_t *newfunc = NULL;
			status = eval(func, environ, &newfunc);
			node_release(func);
			func = newfunc;
			if(status != EVAL_OK) {
				goto finish;
			}
		}

		switch(node_type(func)) {
		case NODE_LIST: {
			/* ensure (lambda (vars) (expr)) */
			traverse(func, "c", &sym);
			traverse(func, "nc", &vars);
			traverse(func, "nnc", &expr);
			if(traverse(func, "nnn", NULL)) {
				status = EVAL_ERR_TOO_MANY_ARGS;
				*output = func;
				goto finish;
			}
			if(node_type(sym) != NODE_SYMBOL ||
			   strcmp(node_name(sym), "lambda")) {
				status = EVAL_ERR_NONLAMBDA_FUNCALL;
				*output = sym;
				goto finish;
			}
			if(node_type(expr) == NODE_NIL) {
				status = EVAL_ERR_MISSING_ARG;
				*output = func;
				goto finish;
			}
			status = lambda_call(vars, args, expr, environ, output);
			break;
		}
		case NODE_BUILTIN:
			status = node_func(func)(args, environ, output);
			break;

		default:
			*output = func;
			status = EVAL_ERR_UNKNOWN_FUNCALL;
			goto finish;
		}
		break;

	case NODE_SYMBOL:
		lookup_stat = environ_lookup(*environ, input, output);
		if(! lookup_stat) {
			*output = input;
			status = EVAL_ERR_UNRESOLVED_SYMBOL;
		}
		break;

	case NODE_VALUE:
	case NODE_BUILTIN:
		*output = node_retain(input);
		break;
	}

finish:
	node_release(args);
	node_release(func);
	node_release(sym);
	node_release(vars);
	node_release(expr);
	
	return status;
}

static char *eval_err_messages[] = {
#define X(A, B) B,
EVAL_ERR_DEFS
#undef X
};

char *eval_err_str(eval_err_t err)
{
	return eval_err_messages[err];
}
