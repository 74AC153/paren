#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "environ.h"
#include "eval.h"

#if 0
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
#endif

eval_err_t lambda_bind(node_t *env_handle, node_t *vars, node_t *args)
{
	node_t *var_curs = NULL, *arg_curs = NULL;
	node_t *kv, *name, *value;
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
		if(environ_keyval_frame(node_handle(env_handle), name, &kv)) {
			node_cons_patch_cdr(kv, value);
		} else {
			environ_add(env_handle, name, value);
		}

		if(early_term) {
			break;
		}
		var_curs = node_cons_cdr(var_curs);
		arg_curs = node_cons_cdr(arg_curs);
	}   

	return EVAL_OK;
}

#if 0
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
		/* (something args...) */
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
				node_droproot(temp); // release temp so we don't leak mem
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
#endif





struct eval_frame_locals {
	uint64_t tailcall;
	node_t *expr_curs;
	node_t *env_handle;
	node_t *newargs;
	node_t *newargs_last;
	node_t *args_curs;
	void *restart;
	node_t *args;
	node_t *test;
	node_t *pass;
	node_t *fail;
	node_t *test_res;
	uint64_t frameadded;
	node_t *in;
	node_t *func;
};

void eval_push_state(
	node_t **bt,
	struct eval_frame_locals *locals,
	node_t *in,
	node_t *env_handle)
{
	node_t *newframe, *oldframe, *temp;

	oldframe = *bt;

	newframe = node_cons_new(locals->func, NULL);
	newframe = node_cons_new(locals->in, newframe);
	temp = node_value_new(locals->frameadded);
	newframe = node_cons_new(temp, newframe);
	newframe = node_cons_new(locals->test_res, newframe);
	newframe = node_cons_new(locals->fail, newframe);
	newframe = node_cons_new(locals->pass, newframe);
	newframe = node_cons_new(locals->test, newframe);
	newframe = node_cons_new(locals->args, newframe);
	temp = node_value_new((int64_t) locals->restart);
	newframe = node_cons_new(temp, newframe);
	newframe = node_cons_new(locals->args_curs, newframe);
	newframe = node_cons_new(locals->newargs_last, newframe);
	newframe = node_cons_new(locals->newargs, newframe);
	newframe = node_cons_new(locals->env_handle, newframe);
	newframe = node_cons_new(locals->expr_curs, newframe);
	temp = node_value_new((int64_t) locals->tailcall);
	newframe = node_cons_new(temp, newframe);

	newframe = node_cons_new(newframe, oldframe);
	node_lockroot(newframe);
	if(oldframe) {
		node_droproot(oldframe);
	}
	*bt = newframe;

	memset(locals, 0, sizeof(*locals));
	locals->in = in;
	locals->env_handle = env_handle;
}

void eval_pop_state(node_t **bt, struct eval_frame_locals *locals)
{
	node_t *prevframe, *temp;

	prevframe = *bt;

	temp = node_cons_car(prevframe);

	locals->tailcall = node_value(node_cons_car(temp));
	temp = node_cons_cdr(temp);

	locals->expr_curs = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->env_handle = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->newargs = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->newargs_last = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->args_curs = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->restart = (void*) node_value(node_cons_car(temp));
	temp = node_cons_cdr(temp);

	locals->args = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->test = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->pass = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->fail = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->test_res = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->frameadded = node_value(node_cons_car(temp));
	temp = node_cons_cdr(temp);

	locals->in = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->func = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	*bt = node_cons_cdr(*bt);
	node_lockroot(*bt);
	node_droproot(prevframe);
}

eval_err_t eval_norec(node_t *in, node_t *env_handle, node_t **out)
{
	eval_err_t status = EVAL_OK;
	struct eval_frame_locals locals = {
		.tailcall = 0,
		.env_handle = env_handle,
		.expr_curs = NULL,
		.newargs = NULL,
		.newargs_last = NULL,
		.args_curs = NULL,
		.restart = NULL,
		.args = NULL,
		.test = NULL,
		.pass = NULL,
		.fail = NULL,
		.test_res = NULL,
		.frameadded = 0,
		.in = in,
		.func = NULL,
	};
	node_t *bt = NULL;
	node_t *result = NULL;

	/* if 'in' is not locked, calls to eval_push_state may cause memory
	   to be made nonroot and likely to be freed before we want it to be */
	/* TODO: enable this */
	//assert(node_islocked(in));

restart:

	switch(node_type(locals.in)) {
	case NODE_NIL:
		result = NULL;
		break;

	case NODE_LAMBDA:
	case NODE_VALUE:
	case NODE_FOREIGN:
	case NODE_LAMBDA_FUNC:
	case NODE_IF_FUNC:
		result = locals.in;
		break;

	case NODE_QUOTE:
		result = node_quote_val(locals.in);
		break;

	case NODE_SYMBOL:
		if(! environ_lookup(node_handle(locals.env_handle),
		                    locals.in,
		                    &result)) {
			result = locals.in;
			status = EVAL_ERR_UNRESOLVED_SYMBOL;
		}
		break;

	case NODE_CONS: 
		/* (symbol args...) */
		locals.args = node_cons_cdr(locals.in);
		if(locals.args && node_type(locals.args) != NODE_CONS) {
			result = locals.args;
			status = EVAL_ERR_EXPECTED_CONS;
			break;
		}

		/* 	status = eval(node_cons_car(input), environ, &func); */
		locals.restart = &&node_cons_post_eval_func;
		eval_push_state(&bt, &locals,
		                node_cons_car(locals.in),
		                locals.env_handle);
		goto restart;
		node_cons_post_eval_func:
		if(status != EVAL_OK) {
			goto node_cons_cleanup;
		}
		locals.func = result;


		switch(node_type(locals.func)) {
		case NODE_FOREIGN:
			status = node_foreign_func(locals.func)(locals.args,
			                                        env_handle,
			                                        &result);
			break;

		case NODE_LAMBDA_FUNC:
			result = node_lambda_new(node_handle(locals.env_handle),
			                         /* vars */
			                         node_cons_car(locals.args), 
			                         /* expr list */
			                         node_cons_cdr(locals.args));
			break;

		case NODE_IF_FUNC:
			/* (if test pass fail) */
			/* status = eval(test, env_handle, &result); */
			locals.restart = &&node_if_post_eval_test;
			eval_push_state(&bt, &locals,
			                node_cons_car(locals.args), env_handle);
			goto restart;
			node_if_post_eval_test:
			if(status != EVAL_OK) {
				goto node_cons_cleanup;
			}
			if(result != NULL) {
				locals.in = node_cons_cdr(locals.args);
			} else {
				locals.in = node_cons_cdr(locals.args);
				if(node_type(locals.in) != NODE_CONS) {
					status = EVAL_ERR_EXPECTED_CONS;
					break;
				}
				locals.in = node_cons_cdr(locals.in);
			}
			if(node_type(locals.in) != NODE_CONS) {
				status = EVAL_ERR_EXPECTED_CONS;
				break;
			}
			locals.in = node_cons_car(locals.in);
			goto restart;
			break;

#if 0
			locals.test = node_cons_car(locals.args);

			result = locals.pass = node_cons_cdr(locals.args);
			if(node_type(locals.pass) != NODE_CONS) {
				status = EVAL_ERR_EXPECTED_CONS;
				break;
			}
			locals.pass = node_cons_car(locals.pass);

			result = locals.fail = node_cons_cdr(node_cons_cdr(locals.args));
			if(node_type(locals.fail) != NODE_CONS) {
				status = EVAL_ERR_EXPECTED_CONS;
				break;
			}
			locals.fail = node_cons_car(locals.fail);


			/* status = eval(test, environ, &test_res); */
			locals.restart = &&node_if_post_eval_test;
			eval_push_state(&bt, &locals,
			                locals.test, env_handle);
			goto restart;
			node_if_post_eval_test:
			if(status != EVAL_OK) {
				goto node_cons_cleanup;
			}
			locals.test_res = result;

			if(locals.test_res != NULL) {
				locals.in = locals.pass;
			} else {
				locals.in = locals.fail;
			}
			node_droproot(locals.test_res);
			goto restart;

			break;
#endif

		case NODE_LAMBDA: 
			/* eval passed arguments in caller's environment */
			locals.newargs_last = NULL;
			locals.newargs = NULL;
			for(locals.args_curs = locals.args;
			    locals.args_curs;
			    locals.args_curs = node_cons_cdr(locals.args_curs)) {

				locals.restart = &&node_lambda_post_args_eval;
				eval_push_state(&bt, &locals,
				                node_cons_car(locals.args_curs),
				                env_handle);
				goto restart;
				node_lambda_post_args_eval:
				if(status != EVAL_OK) {
					node_droproot(locals.newargs);
					goto node_cons_cleanup;
				}

				if(locals.newargs == NULL) {
					locals.newargs = node_cons_new(result, NULL);
					node_lockroot(locals.newargs);
					locals.newargs_last = locals.newargs;
				} else {
					node_cons_patch_cdr(locals.newargs_last,
					                    node_cons_new(result, NULL));
					locals.newargs_last = node_cons_cdr(locals.newargs_last);
				}
			}

			/* NB: when invoking a lambda call, the stackframe is a child to
			   the the environment that the lambda was defined in, not a child
			   to the environment that the lambda was called from */
			if(! locals.tailcall) {
				node_t *lambda_env = node_lambda_env(locals.func);
				locals.env_handle = node_handle_new(lambda_env);
				node_lockroot(locals.env_handle);
				environ_pushframe(locals.env_handle);
				locals.frameadded = true;
			}

			/* bind / update variables to passed eval'd arguments */
			status = lambda_bind(locals.env_handle,
			                     node_lambda_vars(locals.func),
			                     locals.newargs);
			node_droproot(locals.newargs);
			locals.newargs = NULL;
			if(status != EVAL_OK) {
				result = locals.func;
				goto node_lambda_cleanup;
			}

			/* eval expr list in new environment */
			locals.expr_curs = node_lambda_expr(locals.func);
			for( ;
			    locals.expr_curs;
			    locals.expr_curs = node_cons_cdr(locals.expr_curs)) {

				if(node_cons_cdr(locals.expr_curs) == NULL) {
					/* tail call: clean up and setup to restart */
					locals.in = node_cons_car(locals.expr_curs);
					locals.tailcall = true;
					goto restart;
				} else {
					/* status = eval(node_cons_car(expr_curs), env,
					                 [ignored]); */
					locals.restart = &&node_lambda_post_exprs_eval;
					eval_push_state(&bt, &locals,
					                node_cons_car(locals.expr_curs),
					                locals.env_handle);
					goto restart;
					node_lambda_post_exprs_eval:
					if(status != EVAL_OK) {
						goto node_cons_cleanup;
					}
					if(!node_islocked(result)) {
						node_droproot(result);
					}
				}
			}

			assert(0); /* not reached */

		node_lambda_cleanup:
			break;

		default:
			result = locals.func;
			status = EVAL_ERR_UNKNOWN_FUNCALL;
			break;
		}
		
	node_cons_cleanup:
		break;

	default:
		assert(false);
	}

	if(locals.frameadded) {
		environ_popframe(locals.env_handle);
		node_droproot(locals.env_handle);
	}

	if(bt) {
		node_droproot(locals.newargs);
		eval_pop_state(&bt, &locals);
		goto *locals.restart;
	}

	*out = result;
	return status;
}
