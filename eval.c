#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

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
		node_forget(newarg);
		return status;
	}

	*result = node_new_list(newarg, rest);

	return EVAL_OK;
}

eval_err_t lambda_bind(node_t **environ, node_t *vars, node_t *args)
{
	node_t *var_curs = NULL, *arg_curs = NULL;
	node_t *kv, *name, *value;
	node_t *env = *environ;
	bool early_term = false;

	var_curs = vars;
	arg_curs = args;
	while(true) {
		/* ( ) */
		if(! var_curs) {
			if(arg_curs) {
				return EVAL_ERR_TOO_MANY_ARGS;
			}
			break;
		/* ( sym ... ) */
		} else if(node_type(var_curs) == NODE_LIST) {
			if(node_type(arg_curs) != NODE_LIST) {
				return EVAL_ERR_MISSING_ARG;
			}
			value = node_child_noref(arg_curs);
			name = node_child_noref(var_curs);
			if(node_type(name) != NODE_SYMBOL) {
				return EVAL_ERR_EXPECTED_SYMBOL;
			}
		/* ( ... . sym ) */
		} else if(node_type(var_curs) == NODE_SYMBOL) {
			value = arg_curs;
			name = var_curs;
			early_term = true;
		} else {
			return EVAL_ERR_EXPECTED_LIST_SYM;
		}

		/* if the symbol already exists in the current frame, update its
		   value, otherwise, add the value to the frame*/
		if(environ_keyvalue_frame(*environ, name, &kv)) {
			node_patch_list_next(kv, value);
		} else {
			environ_add(&env, name, value);
		}

		if(early_term) {
			break;
		}
		var_curs = node_next_noref(var_curs);
		arg_curs = node_next_noref(arg_curs);
	}   

	*environ = env;
	return EVAL_OK;
}

eval_err_t eval(node_t *input, node_t **environ, node_t **output)
{
	node_t *expr_curs, *temp, *func, *args, *newargs;
	eval_err_t status;
	bool tailcall = false;
	bool frameadded = false;

eval_tailcall_restart:
#if defined(EVAL_TRACING)
	printf("eval of:");
	node_print_pretty(input);
	printf("\n");
#endif

	expr_curs = NULL;
	temp = NULL;
	func = NULL;
	args = NULL;
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
		args = node_next_noref(input);
		if(node_type(args) != NODE_LIST) {
			*output = args;
			status = EVAL_ERR_EXPECTED_LIST;
			break;
		}

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
			args = newargs;

			/* bind variables to passed eval'd arguments */
			if(! tailcall) {
				environ_pushframe(environ);
				frameadded = true;
			}
			status = lambda_bind(environ, node_lambda_vars_noref(func), args);
			if(status != EVAL_OK) {
				*output = func;
				goto node_lambda_cleanup;
			}

			/* eval expr list in new environment */
			expr_curs = node_lambda_expr_noref(func);
			for(temp = NULL;
			    expr_curs;
			    expr_curs = node_next_noref(expr_curs)) {
				node_forget(temp);
				if(node_next_noref(expr_curs) == NULL) {
					/* tail call: clean up and setup to restart */
					input = node_child_noref(expr_curs);
					tailcall = true;
					goto eval_tailcall_restart;
				} else {
					status = eval(node_child_noref(expr_curs), environ, &temp);
					if(status != EVAL_OK) {
						break;
					}
				}
			}

			/* return value of last eval */
			*output = temp;
		node_lambda_cleanup:
			break;
		}

		case NODE_LAMBDA_FUNC:
			*output = node_new_lambda(*environ,
			                          node_child_noref(args), /* vars */
			                          node_next_noref(args)); /* expr list */
			break;

		case NODE_IF_FUNC: {
			node_t *test = NULL, *pass = NULL, *fail = NULL, *test_res = NULL;
			test = node_child_noref(args);
	
			*output = pass = node_next_noref(args);
			if(node_type(pass) != NODE_LIST) {
				status = EVAL_ERR_EXPECTED_LIST;
				break;
			}
			pass = node_child_noref(pass);

			*output = fail = node_next_noref(node_next_noref(args));
			if(node_type(fail) != NODE_LIST) {
				status = EVAL_ERR_EXPECTED_LIST;
				break;
			}
			fail = node_child_noref(fail);

			status = eval(test, environ, &test_res);
			if(status != EVAL_OK) {
				*output = test;
				break;
			}

			if(test_res != NULL) {
				input = pass;
			} else {
				input = fail;
			}
			node_forget(test_res);

			goto eval_tailcall_restart;
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
		*output = node_quote_val_noref(input);
		break;

	case NODE_LAMBDA:
	case NODE_VALUE:
	case NODE_BUILTIN:
		*output = input;
		break;

	default:
		assert(false);
	}
	
	if(frameadded) {
		environ_popframe(environ);
	}

	return status;
}
