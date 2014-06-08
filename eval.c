#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "libc_custom.h"
#include "environ.h"
#include "frame.h"
#include "eval.h"
#include "dbgtrace.h"
#include "traceclass.h"

eval_err_t lambda_bind(node_t *env_handle, node_t *vars, node_t *args)
{
	/* walk down the lists pointed to by vars and args in unison, and add
	   bindings from each element in vars to each element in args into
	   env_handle */

	node_t *var_curs = NULL, *arg_curs = NULL;
	node_t *kv, *name, *value;
	bool early_term = false;

	var_curs = vars;
	arg_curs = args;
	while(true) {
		/* get next name and value */
		if(! var_curs) {
			/* ( ) */
			if(arg_curs) {
				/* still have args to bind but no more vars to bind them to */
				return eval_err(EVAL_ERR_TOO_MANY_ARGS);
			}
			break;
		} else if(node_type(var_curs) == NODE_CONS) {
			/* ( sym ... ) */
			if(node_type(arg_curs) != NODE_CONS) {
				return eval_err(EVAL_ERR_MISSING_ARG);
			}
			value = node_cons_car(arg_curs);
			name = node_cons_car(var_curs);
			if(node_type(name) != NODE_SYMBOL) {
				return eval_err(EVAL_ERR_EXPECTED_SYMBOL);
			}
		} else if(node_type(var_curs) == NODE_SYMBOL) {
			/* ( ... . sym ) */
			value = arg_curs;
			name = var_curs;
			/* the last symbol is bound to the rest of the arg list, we can
			   stop walking the arg list after we add this binding */
			early_term = true;
		} else {
			/* should only be given a list of symbols */
			return eval_err(EVAL_ERR_EXPECTED_CONS_SYM);
		}

		/* if the symbol with same name already exists in the current frame,
		   update its value, otherwise, add the name/value pair to the frame */
		if(environ_keyval_frame(env_handle, name, &kv)) {
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

// flags that control type of execution in eval
#define STATE_FLAG_TAILCALL   0x1
#define STATE_FLAG_FRAMEADDED 0x2
#define STATE_FLAG_NEW_INPUT  0x4

enum local_id {
	LOCAL_ID_STATE,
	LOCAL_ID_RESTART,
	LOCAL_ID_CURSOR,
	LOCAL_ID_NA_LAST,
	LOCAL_ID_NEWARGS,
	LOCAL_ID_ENV_HDL,
	LOCAL_ID_FUNC,
	LOCAL_ID_INPUT,
	LOCAL_ID_MAX
};

static value_t locals_state_get(node_t **locals)
{
	if(node_handle(locals[LOCAL_ID_STATE])) {
		return node_value(node_handle(locals[LOCAL_ID_STATE]));
	} else {
		return 0;
	}
}

static void locals_state_set(memory_state_t *s, node_t **locals, value_t v)
{
	node_handle_update(locals[LOCAL_ID_STATE],
	                   node_value_new(s, v));
}

static void *locals_restart_get(node_t **locals)
{
	if(node_handle(locals[LOCAL_ID_RESTART])) {
		return node_blob_addr(node_handle(locals[LOCAL_ID_RESTART]));
	} else {
		return NULL;
	}
}

static void locals_restart_set(memory_state_t *s, node_t **locals, void *addr)
{
	node_handle_update(locals[LOCAL_ID_RESTART],
	                   node_blob_new(s, addr, NULL, 0));
}

eval_err_t eval(
	memory_state_t *ms,
	node_t *env_handle,
	node_t *in_handle,
	node_t *out_handle)
{
	DBGSTMT(char buf[21]);
	DBGSTMT(char buf2[21]);
	DBGSTMT(char buf3[21]);
	eval_err_t status = EVAL_OK;

	/* saved local variables */
	node_t *locals[LOCAL_ID_MAX] = { NULL };
	node_t *newvals[LOCAL_ID_MAX] = { NULL };

	/* accessor macros for state held at each eval step */
	#define _STATE locals_state_get(locals)
	#define _RESTART locals_restart_get(locals)
	#define _CURSOR node_handle(locals[LOCAL_ID_CURSOR])
	#define _NEWARGS_LAST node_handle(locals[LOCAL_ID_NA_LAST])
	#define _NEWARGS node_handle(locals[LOCAL_ID_NEWARGS])
	#define _ENV_HDL node_handle(locals[LOCAL_ID_ENV_HDL])
	#define _FUNC node_handle(locals[LOCAL_ID_FUNC])
	#define _INPUT node_handle(locals[LOCAL_ID_INPUT])

	#define _SET_STATE(x) locals_state_set(ms, locals, (x))
	#define _SET_RESTART(x) locals_restart_set(ms, locals, (x))
	#define _SET_CURSOR(x) node_handle_update(locals[LOCAL_ID_CURSOR], (x))
	#define _SET_NA_LAST(x) node_handle_update(locals[LOCAL_ID_NA_LAST], (x))
	#define _SET_NEWARGS(x) node_handle_update(locals[LOCAL_ID_NEWARGS], (x))
	#define _SET_ENV_HDL(x) node_handle_update(locals[LOCAL_ID_ENV_HDL], (x))
	#define _SET_FUNC(x) node_handle_update(locals[LOCAL_ID_FUNC], (x))
	#define _SET_INPUT(x) node_handle_update(locals[LOCAL_ID_INPUT], (x))

	/* long-lived local variables */
	node_t *frame_hdl;
	node_t *result_handle;
	size_t bt_depth = 0;
	size_t env_depth = 0;

	/* temps -- must be restored after recursive calls */
	node_t *_args;
	node_t *keyval;
	node_t *temp;

	assert(node_type(in_handle) == NODE_HANDLE);
	assert(node_type(env_handle) == NODE_HANDLE);
	assert(node_type(out_handle) == NODE_HANDLE);

	/* frame list holds execution "stack" of eval
	   -- can be saved / unwound independent of C runtime function calls
	      to eval() */
	frame_init(ms, locals, LOCAL_ID_MAX);
	/* holds the actual frame variables for current loop through eval() */
	bzero_custom(newvals, sizeof(newvals));

	frame_hdl = node_lockroot(node_handle_new(ms, NULL));
	result_handle = node_lockroot(node_handle_new(ms, NULL));

	_SET_INPUT(node_handle(in_handle));
	_SET_ENV_HDL(env_handle);

restart:

	DBGTRACE(TC_EVAL, fmt_u64d(buf, bt_depth),
	         " eval enter ", fmt_ptr(buf2, _INPUT), ": -> ");
	DBGRUN(TC_EVAL,
	       { node_print_pretty_stream(dbgtrace_getstream(), _INPUT, false); });
	DBGTRACE(TC_EVAL, "\n");

	switch(node_type(_INPUT)) {
	case NODE_UNINITIALIZED:
	case NODE_HANDLE:
		assert(false);

	case NODE_NIL:
	case NODE_LAMBDA:
	case NODE_VALUE:
	case NODE_FOREIGN:
	case NODE_CONTINUATION:
	case NODE_SPECIAL_FUNC:
	case NODE_BLOB:
		node_handle_update(result_handle, _INPUT);
		goto finish;

	case NODE_SYMBOL:
		if(! environ_lookup(_ENV_HDL,
		                    _INPUT,
		                    &temp)) {
			node_handle_update(result_handle, _INPUT);
			status = eval_err(EVAL_ERR_UNRESOLVED_SYMBOL);
		} else {
			node_handle_update(result_handle, temp);
		}
		goto finish;

	case NODE_CONS: 
		/* cons processing follows below... */
		break;
	}


	/* (symbol args...) */
	_args = node_cons_cdr(_INPUT);
	if(_args && node_type(_args) != NODE_CONS) {
		node_handle_update(result_handle, _args);
		status = eval_err(EVAL_ERR_EXPECTED_CONS);
		goto node_cons_cleanup;
	}

	/* 	status = eval(node_cons_car(input), environ, &func); */
	_SET_RESTART(&&node_cons_post_eval_func);
	newvals[LOCAL_ID_ENV_HDL] = _ENV_HDL;
	newvals[LOCAL_ID_INPUT] = node_cons_car(_INPUT);
	frame_push(frame_hdl, locals, newvals, LOCAL_ID_MAX);
	bt_depth++;
	goto restart;
	node_cons_post_eval_func:
	if(status != EVAL_OK) {
		goto node_cons_cleanup;
	}
	_SET_FUNC(node_handle(result_handle));

	_args = node_cons_cdr(_INPUT); /* restore */

	if(node_type(_FUNC) == NODE_SPECIAL_FUNC) {
		switch(node_special_func(_FUNC)) {
		case SPECIAL_IF:
			/* (if args),  args -> (test pass fail) */
			/* status = eval(test, locals.env_handle, &result); */
			_SET_RESTART(&&node_if_post_eval_test);
			newvals[LOCAL_ID_ENV_HDL] = _ENV_HDL;
			newvals[LOCAL_ID_INPUT] = node_cons_car(_args);
			frame_push(frame_hdl, locals, newvals, LOCAL_ID_MAX);
			bt_depth++;
			goto restart;
			node_if_post_eval_test:
			if(status != EVAL_OK) {
				goto node_cons_cleanup;
			}

			_args =	node_cons_cdr(_INPUT); /* restore */

			/* select which expr to eval based on result */
			if(node_handle(result_handle) != NULL) {
				node_handle_update(result_handle, NULL);
				_SET_INPUT(node_cons_cdr(_args));
			} else {
				temp = node_cons_cdr(_args);
				if(node_type(temp) != NODE_CONS) {
					node_handle_update(result_handle, temp);
					status = eval_err(EVAL_ERR_EXPECTED_CONS);
					goto node_cons_cleanup;
				}
				_SET_INPUT(node_cons_cdr(temp));
			}
			if(node_type(_INPUT) != NODE_CONS) {
				node_handle_update(result_handle, _INPUT);
				status = eval_err(EVAL_ERR_EXPECTED_CONS);
				goto node_cons_cleanup;
			}

			if(_STATE & STATE_FLAG_NEW_INPUT) {
				/* we'll never return to this stack frame, so we must
				   do this here */
				_SET_INPUT(NULL);
				_SET_STATE(_STATE &~ STATE_FLAG_NEW_INPUT);
			}

			_SET_INPUT(node_cons_car(_INPUT));
			_SET_FUNC(NULL); /* b/c cons_cleanup not done */
			_SET_STATE(_STATE | STATE_FLAG_TAILCALL);
			/* tailcall */
			goto restart;

		case SPECIAL_LAMBDA:
			temp = node_lambda_new(ms, node_handle(_ENV_HDL),
			                       /* vars */
			                       node_cons_car(_args), 
			                       /* expr list */
			                       node_cons_cdr(_args));
			node_handle_update(result_handle, temp);
			goto node_cons_cleanup;

		case SPECIAL_QUOTE:
			if(node_cons_cdr(_args)) {
				node_handle_update(result_handle, node_cons_cdr(_args));
				status = eval_err(EVAL_ERR_TOO_MANY_ARGS);
				goto node_cons_cleanup;
			}
			temp = node_cons_car(_args);
			node_handle_update(result_handle, temp);
			goto node_cons_cleanup;

		case SPECIAL_MK_CONT:
			/* generate new cons with first arg as passed func, second arg
			   as result of node_cont_new(), then do a tail call. */
			/* TODO: node_cons_car(_args) should be eval'd */
			temp = node_cons_new(ms, node_cont_new(ms, node_handle(frame_hdl)),
			                     NULL);
			temp = node_cons_new(ms, node_cons_car(_args), temp);
			_SET_INPUT(temp);
			_SET_STATE(_STATE | STATE_FLAG_NEW_INPUT);
			_SET_FUNC(NULL);
			goto restart;

		case SPECIAL_DEF:
		case SPECIAL_SET:
			/* return the symbol that was added to environment */
			_SET_CURSOR(node_cons_car(_args));
			if(node_type(_CURSOR) != NODE_SYMBOL) {
				node_handle_update(result_handle, _CURSOR);
				status = eval_err(EVAL_ERR_EXPECTED_SYMBOL);
				goto node_cons_cleanup;
			}
		
			/* eval passed value */
			if(node_type(node_cons_cdr(_args)) != NODE_CONS) {
				node_handle_update(result_handle, node_cons_cdr(_args));
				return eval_err(EVAL_ERR_EXPECTED_CONS);
			}

			/* status = eval_norec(val, env_handle, &newval); */
			_SET_RESTART(&&node_special_def_set_post_eval_func);
			newvals[LOCAL_ID_ENV_HDL] = _ENV_HDL;
			newvals[LOCAL_ID_INPUT] = node_cons_car(node_cons_cdr(_args));
			frame_push(frame_hdl, locals, newvals, LOCAL_ID_MAX);
			bt_depth++;
			goto restart;
			node_special_def_set_post_eval_func:
			if(status != EVAL_OK) {
				goto node_cons_cleanup;
			}
		
			if(node_special_func(_FUNC) == SPECIAL_DEF) {
				temp = node_handle(result_handle);
				environ_add(_ENV_HDL, _CURSOR, temp);
			} else {
				if(!environ_keyval(_ENV_HDL,
				                   _CURSOR,
				                   &keyval)) {
					node_handle_update(result_handle, _CURSOR);
					status = eval_err(EVAL_ERR_UNRESOLVED_SYMBOL);
					goto node_cons_cleanup;
				}
				temp = node_handle(result_handle);
				node_cons_patch_cdr(keyval, temp);
			}
			node_handle_update(result_handle, _CURSOR);
			goto node_cons_cleanup;
			case SPECIAL_DEFINED:
			case SPECIAL_EVAL:
				/* handled below */
				break;
		}
	}

	/* eval passed arguments in caller's environment */
	_SET_NA_LAST(NULL);
	_SET_NEWARGS(NULL);
	for(_SET_CURSOR(node_cons_cdr(_INPUT));
	    _CURSOR;
		_SET_CURSOR(node_cons_cdr(_CURSOR))) {

		_SET_RESTART(&&node_cons_post_args_eval);
		newvals[LOCAL_ID_ENV_HDL] = _ENV_HDL;
		newvals[LOCAL_ID_INPUT] = node_cons_car(_CURSOR);
		frame_push(frame_hdl, locals, newvals, LOCAL_ID_MAX);
		bt_depth++;
		goto restart;
		node_cons_post_args_eval:
		if(status != EVAL_OK) {
			_SET_NA_LAST(NULL);
			_SET_NEWARGS(NULL);
			goto node_lambda_cleanup;
		}

		temp = node_handle(result_handle);
		if(_NEWARGS == NULL) {
			_SET_NEWARGS(node_cons_new(ms, temp, NULL));
			_SET_NA_LAST(_NEWARGS);
		} else {
			node_cons_patch_cdr(_NEWARGS_LAST,
			                    node_cons_new(ms, temp, NULL));
			_SET_NA_LAST(node_cons_cdr(_NEWARGS_LAST));
		}
	}

	switch(node_type(_FUNC)) {
	case NODE_SPECIAL_FUNC:
		switch(node_special_func(_FUNC)) {
		case SPECIAL_DEFINED:
			if(_NEWARGS) {
				temp = node_cons_car(_NEWARGS);
				if(node_type(temp) == NODE_SYMBOL
				   && environ_lookup(_ENV_HDL,
				                     temp, NULL)) {
					node_handle_update(result_handle, node_value_new(ms, 1));
				}
			}
			goto node_cons_cleanup;
		case SPECIAL_EVAL:
			/* restart with new input */
			_SET_INPUT(node_cons_car(_NEWARGS));
			goto restart;		
		default:
			assert(false);
		}
	case NODE_FOREIGN:
		status = node_foreign_func(_FUNC)(ms, _NEWARGS, _ENV_HDL, &temp);
		node_handle_update(result_handle, temp);
		goto node_cons_cleanup;

	case NODE_CONTINUATION:
		node_handle_update(frame_hdl, node_cont(_FUNC));
		node_handle_update(result_handle, node_cons_car(_NEWARGS));
		goto node_cons_cleanup;

	case NODE_LAMBDA: 
		/* NB: when invoking a lambda call, the stackframe is a child to
		   the the environment that the lambda was defined in, not a child
		   to the environment that the lambda was called from */
		if(! (_STATE & STATE_FLAG_TAILCALL)) {
			node_t *lambda_env = node_lambda_env(_FUNC);
			node_t *newhandle = node_handle_new(ms, lambda_env);
			_SET_ENV_HDL(newhandle);
			environ_pushframe(_ENV_HDL);
			env_depth++;

			DBGTRACELN(TC_EVAL, fmt_u64d(buf, bt_depth),
			           " eval env_depth++ -> ", fmt_u64(buf2, env_depth));

			_SET_STATE(_STATE | STATE_FLAG_FRAMEADDED);
		}

		/* bind / update variables to passed eval'd arguments */
		status = lambda_bind(_ENV_HDL,
		                     node_lambda_vars(_FUNC),
		                     _NEWARGS);
		if(status != EVAL_OK) {
			node_handle_update(result_handle, _FUNC);
			goto node_lambda_cleanup;
		}

		/* eval expr list in new environment */
		for(_SET_CURSOR(node_lambda_expr(_FUNC));
		    _CURSOR;
		    _SET_CURSOR(node_cons_cdr(_CURSOR))) {

			if(node_cons_cdr(_CURSOR) == NULL) {
				/* tail call: clean up and setup to restart */
				if(_STATE & STATE_FLAG_NEW_INPUT) {
					/* we'll never return to this stack frame, so we must
					   do this here */
					_SET_INPUT(NULL);
					_SET_STATE(_STATE & ~STATE_FLAG_NEW_INPUT);
				}
				_SET_INPUT(node_cons_car(_CURSOR));
				_SET_STATE(_STATE | STATE_FLAG_TAILCALL);
				/* because cons_cleanup will not be done */
				_SET_NA_LAST(NULL);
				_SET_NEWARGS(NULL);
				_SET_FUNC(NULL);
				/* tailcall */
				goto restart;
			} else {
				/* status = eval(node_cons_car(cursor), env, [ignored]); */
				_SET_RESTART(&&node_lambda_post_exprs_eval);
				newvals[LOCAL_ID_ENV_HDL] = _ENV_HDL;
				newvals[LOCAL_ID_INPUT] = node_cons_car(_CURSOR);
				frame_push(frame_hdl, locals, newvals, LOCAL_ID_MAX);
				bt_depth++;
				goto restart;
				node_lambda_post_exprs_eval:
				if(status != EVAL_OK) {
					goto node_lambda_cleanup;
				}
				node_handle_update(result_handle, NULL);
			}
		}

		assert(0); /* not reached */

	node_lambda_cleanup:
		break;

	default:
		node_handle_update(result_handle, _FUNC);
		status = eval_err(EVAL_ERR_UNKNOWN_FUNCALL);
		break;
	}
	
node_cons_cleanup:
	_SET_FUNC(NULL);
	_SET_NEWARGS(NULL);
	_SET_NA_LAST(NULL);

finish:
	if(_STATE & STATE_FLAG_FRAMEADDED) {
		environ_popframe(_ENV_HDL);
		env_depth--;

		DBGTRACELN(TC_EVAL, fmt_u64d(buf, bt_depth),
		           " eval env_depth-- -> ", fmt_u64(buf2, env_depth));

		_SET_ENV_HDL(NULL);
	}


	if(node_handle(frame_hdl)) {
		_SET_NEWARGS(NULL);
		_SET_NA_LAST(NULL);
		if(_STATE & STATE_FLAG_NEW_INPUT) {
			_SET_INPUT(NULL);
		}

		DBGTRACE(TC_EVAL, fmt_u64d(buf, bt_depth), " eval leave ", 
		         fmt_ptr(buf2, _INPUT), ": <- ");
		DBGRUN(TC_EVAL, {
			node_print_pretty_stream(dbgtrace_getstream(), _INPUT, false);
		});
		DBGTRACE(TC_EVAL, " ... ");
		DBGRUN(TC_EVAL, {
			node_print_pretty_stream(dbgtrace_getstream(), 
			                         node_handle(result_handle),
			                         false);
		});
		DBGTRACE(TC_EVAL, "\n");

		frame_pop(frame_hdl, locals, LOCAL_ID_MAX);
		bt_depth--;
		assert(_RESTART);
		goto *_RESTART;
	}


	node_handle_update(out_handle, node_handle(result_handle));
	node_droproot(frame_hdl);
	node_droproot(result_handle);

	frame_deinit(locals, LOCAL_ID_MAX);

	return status;

	#undef _STATE
	#undef _RESTART
	#undef _CURSOR
	#undef _NEWARGS_LAST
	#undef _NEWARGS
	#undef _ENV_HDL
	#undef _FUNC
	#undef _INPUT
	#undef _SET_STATE
	#undef _SET_RESTART
	#undef _SET_CURSOR
	#undef _SET_NA_LAST
	#undef _SET_NEWARGS
	#undef _SET_ENV_HDL
	#undef _SET_FUNC
	#undef _SET_INPUT
}
