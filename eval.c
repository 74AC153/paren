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

	status = eval(node_cons_car(args), environ, &newarg);
	if(status != EVAL_OK) {
		*result = newarg;
		return status;
	}

	status = map_eval(node_cons_cdr(args), environ, &rest);
	if(status != EVAL_OK) {
		*result = rest;
		node_droproot(newarg);
		return status;
	}

	*result = node_cons_new(newarg, rest);

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
		if(! var_curs) {
			/* ( ) */
			if(arg_curs) {
				/* still have args to bind but no more vars to bind them to */
				return EVAL_ERR_TOO_MANY_ARGS;
			}
			break;
		} else if(node_type(var_curs) == NODE_CONS) {
			/* ( sym ... ) */
			if(node_type(arg_curs) != NODE_CONS) {
				return EVAL_ERR_MISSING_ARG;
			}
			value = node_cons_car(arg_curs);
			name = node_cons_car(var_curs);
			if(node_type(name) != NODE_SYMBOL) {
				return EVAL_ERR_EXPECTED_SYMBOL;
			}
		} else if(node_type(var_curs) == NODE_SYMBOL) {
			/* ( ... . sym ) */
			value = arg_curs;
			name = var_curs;
			/* the last symbol is bound to the rest of the arg list, we can
			   stop walking the arg list */
			early_term = true;
		} else {
			/* should only be given a list of symbols */
			return EVAL_ERR_EXPECTED_CONS_SYM;
		}

		/* if the symbol already exists in the current frame, update its
		   value, otherwise, add the value to the frame */
		if(environ_keyvalue_frame(*environ, name, &kv)) {
			node_cons_patch_cdr(kv, value);
		} else {
			environ_add(&env, name, value);
		}

		if(early_term) {
			break;
		}
		var_curs = node_cons_cdr(var_curs);
		arg_curs = node_cons_cdr(arg_curs);
	}   

	*environ = env;
	return EVAL_OK;
}

eval_err_t eval(node_t *input, node_t **environ, node_t **output)
{
	node_t *expr_curs, *temp, *func, *args, *newargs, *lambda_environ;
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

	case NODE_CONS: {
		/* (symbol args...) */
		args = node_cons_cdr(input);
		if(node_type(args) != NODE_CONS) {
			*output = args;
			status = EVAL_ERR_EXPECTED_CONS;
			break;
		}

		status = eval(node_cons_car(input), environ, &func);
		if(status != EVAL_OK) {
			goto node_list_cleanup;
		}

		switch(node_type(func)) {
		case NODE_LAMBDA: {
			/* eval passed arguments in caller's environment */
			status = map_eval(args, environ, &newargs);
			if(status != EVAL_OK) {
				*output = newargs;
				goto node_lambda_cleanup;
			}
			args = newargs;

			/* NB: when invoking a lambda call, the stackframe is a child to
			   the the environment that the lambda was defined in, not a child
			   to the environment that the lambda was called from */
			if(! tailcall) {
				lambda_environ = node_lambda_env(func);
				environ = &lambda_environ;
				environ_pushframe(environ);
				frameadded = true;
			}
			/* bind / update variables to passed eval'd arguments */
			status = lambda_bind(environ, node_lambda_vars(func), args);
			node_droproot(args);
			if(status != EVAL_OK) {
				*output = func;
				goto node_lambda_cleanup;
			}

			/* eval expr list in new environment */
			expr_curs = node_lambda_expr(func);
			for(temp = NULL;
			    expr_curs;
			    expr_curs = node_cons_cdr(expr_curs)) {
				node_droproot(temp);
				if(node_cons_cdr(expr_curs) == NULL) {
					/* tail call: clean up and setup to restart */
					input = node_cons_car(expr_curs);
					tailcall = true;
					goto eval_tailcall_restart;
				} else {
					status = eval(node_cons_car(expr_curs), environ, &temp);
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
			*output = node_lambda_new(*environ,
			                          node_cons_car(args), /* vars */
			                          node_cons_cdr(args)); /* expr list */
			break;

		case NODE_IF_FUNC: {
			node_t *test = NULL, *pass = NULL, *fail = NULL, *test_res = NULL;
			test = node_cons_car(args);
	
			*output = pass = node_cons_cdr(args);
			if(node_type(pass) != NODE_CONS) {
				status = EVAL_ERR_EXPECTED_CONS;
				break;
			}
			pass = node_cons_car(pass);

			*output = fail = node_cons_cdr(node_cons_cdr(args));
			if(node_type(fail) != NODE_CONS) {
				status = EVAL_ERR_EXPECTED_CONS;
				break;
			}
			fail = node_cons_car(fail);

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
			node_droproot(test_res);

			goto eval_tailcall_restart;
			break;
		}

		case NODE_FOREIGN:
			status = node_foreign_func(func)(args, environ, output);
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
		*output = node_quote_val(input);
		break;

	case NODE_LAMBDA:
	case NODE_VALUE:
	case NODE_FOREIGN:
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
