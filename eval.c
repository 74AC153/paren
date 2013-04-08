#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "environ.h"
#include "eval.h"

eval_err_t map_eval(node_t *args, node_t **environ, node_t **result)
{
	eval_err_t status;
	node_t *newarg = NULL, *rest = NULL;

	if(!args) {
		*result = NULL;
		return EVAL_OK;
	}

	status = eval(node_child_noref(args), environ, &newarg);
	if(status != EVAL_OK) {
		*result = newarg;
		return status;
	}

	status = map_eval(node_next_noref(args), environ, &rest);
	if(status != EVAL_OK) {
		*result = rest;
		node_release(newarg);
		return status;
	}

	*result = node_retain(node_new_list(newarg, rest));
	node_release(newarg);
	node_release(rest);

	return EVAL_OK;
}

eval_err_t lambda_bind(node_t **environ, node_t *vars, node_t *args)
{
	node_t *var_curs = NULL, *arg_curs = NULL;
	node_t *name, *value;
	node_t *env = *environ;
	bool early_term = false;

	node_retain(*environ);

	var_curs = vars;
	arg_curs = args;
	while(true) {
		/* ( ) */
		if(! var_curs) {
			if(arg_curs) {
				node_release(env);
				return EVAL_ERR_TOO_MANY_ARGS;
			}
			break;
		/* ( sym ... ) */
		} else if(node_type(var_curs) == NODE_LIST) {
			if(node_type(arg_curs) != NODE_LIST) {
				node_release(env);
				return EVAL_ERR_MISSING_ARG;
			}
			value = node_child_noref(arg_curs);
			name = node_child_noref(var_curs);
			if(node_type(name) != NODE_SYMBOL) {
				node_release(env);
				return EVAL_ERR_EXPECTED_SYMBOL;
			}
		/* ( ... . sym ) */
		} else if(node_type(var_curs) == NODE_SYMBOL) {
			value = arg_curs;
			name = var_curs;
			early_term = true;
		} else {
			node_release(env);
			return EVAL_ERR_EXPECTED_LIST_SYM;
		}

		environ_add(&env, name, value);

		if(early_term) {
			break;
		}
		var_curs = node_next_noref(var_curs);
		arg_curs = node_next_noref(arg_curs);
	}   

	node_release(*environ);
	*environ = env;
	return EVAL_OK;
}

eval_err_t eval(node_t *input, node_t **environ, node_t **output)
{
	node_t *expr_curs, *temp, *func, *args, *newenv, *newargs;
	eval_err_t status;

eval_tailcall_restart:

	expr_curs = NULL;
	temp = NULL;
	func = NULL;
	args = NULL;
	newenv = NULL;
	newargs = NULL;
	status = EVAL_OK;

	/* not forcing this necessitates messy refcounting logic */
	assert(!input || *output != input);

	switch(node_type(input)) {
	case NODE_NIL:
		*output = NULL;
		break;

	case NODE_LIST: {
		/* (symbol args...) */
		args = node_retain(node_next_noref(input));

		status = eval(node_child_noref(input), environ, &func);
		if(status != EVAL_OK) {
			goto node_list_cleanup;
		}

		switch(node_type(func)) {
		case NODE_LAMBDA: {
			/* eval passed arguments */
			status = map_eval(args, environ, &newargs);
			if(status != EVAL_OK) {
				*output = newargs;
				goto node_lambda_cleanup;
			}
			node_release(args);
			args = newargs;

			/* bind variables to passed eval'd arguments */
			newenv = node_retain(*environ);
			status = lambda_bind(&newenv, node_lambda_vars_noref(func), args);
			if(status != EVAL_OK) {
				*output = func;
				goto node_lambda_cleanup;
			}

			/* eval expr list in new environment */
			expr_curs = node_lambda_expr_noref(func);
			for(temp = NULL;
			    expr_curs;
			    expr_curs = node_next_noref(expr_curs)) {
				node_release(temp);
				if(node_next_noref(expr_curs) == NULL) {
					/* tail call: clean up and setup to restart */
					node_release(*environ);
					*environ = newenv;
					input = node_child_noref(expr_curs);
					node_release(args);
					node_release(func);
					goto eval_tailcall_restart;
				} else {
					status = eval(node_child_noref(expr_curs), &newenv, &temp);
					if(status != EVAL_OK) {
						break;
					}
				}
			}

			/* return value of last eval */
			*output = temp;
		node_lambda_cleanup:
			node_release(newenv);
			break;
		}

		case NODE_BUILTIN:
			status = node_func(func)(args, environ, output);
			break;

		default:
			*output = func;
			status = EVAL_ERR_UNKNOWN_FUNCALL;
			break;
		}
	node_list_cleanup:
		node_release(func);
		node_release(args);
		break;
	}

	case NODE_SYMBOL: {
		if(! environ_lookup(*environ, input, output)) {
			*output = input;
			status = EVAL_ERR_UNRESOLVED_SYMBOL;
		}
		break;
	}

	case NODE_QUOTE:
		*output = node_retain(node_quote_val_noref(input));
		break;

	case NODE_LAMBDA:
	case NODE_VALUE:
	case NODE_BUILTIN:
		*output = node_retain(input);
		break;

	default:
		assert(false);
	}
	
	return status;
}
