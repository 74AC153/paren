#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "environ.h"
#include "eval.h"

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

struct eval_frame_locals {
	uint64_t tailcall;
	uint64_t frameadded;
	void *restart;
	node_t *cursor;
	node_t *newargs_last;

	node_t *newargs; 
	node_t *env_handle;
	node_t *func;
	node_t *in;
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
	temp = node_value_new((int64_t) locals->restart);
	newframe = node_cons_new(temp, newframe);
	newframe = node_cons_new(locals->newargs, newframe);
	newframe = node_cons_new(locals->newargs_last, newframe);
	newframe = node_cons_new(locals->env_handle, newframe);
	newframe = node_cons_new(locals->cursor, newframe);
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

	locals->cursor = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->env_handle = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->newargs = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->newargs_last = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->restart = (void*) node_value(node_cons_car(temp));
	temp = node_cons_cdr(temp);

	locals->frameadded = node_value(node_cons_car(temp));
	temp = node_cons_cdr(temp);

	locals->in = node_cons_car(temp);
	temp = node_cons_cdr(temp);

	locals->func = node_cons_car(temp);
	temp = node_cons_cdr(temp);


	node_lockroot(locals->newargs);
	node_lockroot(locals->func);
	node_lockroot(locals->env_handle);


	*bt = node_cons_cdr(*bt);
	node_lockroot(*bt);
	node_droproot(prevframe);
}

eval_err_t eval_norec(node_t *in, node_t *env_handle, node_t **out)
{
	eval_err_t status = EVAL_OK;

	/* saved local variables */
	struct eval_frame_locals locals = {
		.tailcall = 0,
		.env_handle = env_handle,
		.cursor = NULL,
		.newargs = NULL,
		.newargs_last = NULL,
		.restart = NULL,
		.frameadded = 0,
		.in = in,
		.func = NULL,
	};

	/* long-lived local variables */
	node_t *bt = NULL;
	node_t *result = NULL;

	/* temps -- must be restored after recursive calls */
	node_t *_args = NULL;

	/* if 'in' is not locked, calls to eval_push_state may cause memory
	   to be made nonroot and likely to be freed before we want it to be */
	/* TODO: enable this */
	//assert(node_islocked(in));

restart:
#if defined(EVAL_TRACING)
	printf("eval of:");
	node_print_pretty(locals.in, false);
	printf("\n");
#endif

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
		_args = node_cons_cdr(locals.in);
		if(_args && node_type(_args) != NODE_CONS) {
			result = _args;
			status = EVAL_ERR_EXPECTED_CONS;
			goto node_cons_cleanup;
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
		node_droproot(locals.func); // we may loop back in a tail call
		locals.func = node_lockroot(result);

		switch(node_type(locals.func)) {
		case NODE_FOREIGN:
			status = node_foreign_func(locals.func)(_args,
			                                        locals.env_handle,
			                                        &result);
			goto node_cons_cleanup;

		case NODE_LAMBDA_FUNC:
			result = node_lambda_new(node_handle(locals.env_handle),
			                         /* vars */
			                         node_cons_car(_args), 
			                         /* expr list */
			                         node_cons_cdr(_args));
			goto node_cons_cleanup;

		case NODE_IF_FUNC:
			/* (if args),  args -> (test pass fail) */
			/* status = eval(test, locals.env_handle, &result); */
			locals.restart = &&node_if_post_eval_test;
			eval_push_state(&bt, &locals,
			                node_cons_car(_args), locals.env_handle);
			goto restart;
			node_if_post_eval_test:
			if(status != EVAL_OK) {
				goto node_cons_cleanup;
			}

			_args =	node_cons_cdr(locals.in); /* restore */

			if(result != NULL) {
				node_droproot(result);
				locals.in = node_cons_cdr(_args);
			} else {
				locals.in = node_cons_cdr(_args);
				if(node_type(locals.in) != NODE_CONS) {
					status = EVAL_ERR_EXPECTED_CONS;
					goto node_cons_cleanup;
				}
				locals.in = node_cons_cdr(locals.in);
			}
			if(node_type(locals.in) != NODE_CONS) {
				status = EVAL_ERR_EXPECTED_CONS;
				goto node_cons_cleanup;
			}
			locals.in = node_cons_car(locals.in);
			node_droproot(locals.func); /* b/c cons_cleanup not done */
			goto restart;
			break;

		case NODE_LAMBDA: 
			/* eval passed arguments in caller's environment */
			locals.newargs_last = NULL;
			locals.newargs = NULL;
			for(locals.cursor = _args;
			    locals.cursor;
			    locals.cursor= node_cons_cdr(locals.cursor)) {

				locals.restart = &&node_lambda_post_args_eval;
				eval_push_state(&bt, &locals,
				                node_cons_car(locals.cursor),
				                locals.env_handle);
				goto restart;
				node_lambda_post_args_eval:
				if(status != EVAL_OK) {
					node_droproot(locals.newargs);
					locals.newargs = NULL;
					locals.newargs_last = NULL;
					goto node_lambda_cleanup;
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
				node_t *newhandle = node_handle_new(lambda_env);
				locals.env_handle = newhandle;
				node_lockroot(locals.env_handle);
				environ_pushframe(locals.env_handle);
				locals.frameadded = true;
			}

			/* bind / update variables to passed eval'd arguments */
			status = lambda_bind(locals.env_handle,
			                     node_lambda_vars(locals.func),
			                     locals.newargs);
			if(status != EVAL_OK) {
				result = locals.func;
				goto node_lambda_cleanup;
			}

			/* eval expr list in new environment */
			locals.cursor = node_lambda_expr(locals.func);
			for( ;
			    locals.cursor ;
			    locals.cursor = node_cons_cdr(locals.cursor)) {

				if(node_cons_cdr(locals.cursor) == NULL) {
					/* tail call: clean up and setup to restart */
					locals.in = node_cons_car(locals.cursor);
					locals.tailcall = true;
					/* b/c cons_cleanup not done */
					node_droproot(locals.newargs);
					locals.newargs = NULL;
					locals.newargs_last = NULL;
					node_droproot(locals.func); 
					locals.func = NULL;
					goto restart;
				} else {
					/* status = eval(node_cons_car(cursor), env,
					                 [ignored]); */
					locals.restart = &&node_lambda_post_exprs_eval;
					eval_push_state(&bt, &locals,
					                node_cons_car(locals.cursor),
					                locals.env_handle);
					goto restart;
					node_lambda_post_exprs_eval:
					if(status != EVAL_OK) {
						goto node_lambda_cleanup;
					}
					if(!node_islocked(result)) {
						node_droproot(result);
						result = NULL;
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
		node_droproot(locals.func);
		locals.func = NULL;
		break;

	default:
		assert(false);
	}

	if(locals.frameadded) {
		environ_popframe(locals.env_handle);
		node_droproot(locals.env_handle);
		locals.env_handle = NULL;
	}

	if(bt) {
		node_droproot(locals.newargs);
		locals.newargs = NULL;
		locals.newargs_last = NULL;
		eval_pop_state(&bt, &locals);
		goto *locals.restart;
	}

	*out = result;
	return status;
}
