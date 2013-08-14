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
			   stop walking the arg list */
			early_term = true;
		} else {
			/* should only be given a list of symbols */
			return eval_err(EVAL_ERR_EXPECTED_CONS_SYM);
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

#define STATE_FLAG_TAILCALL   0x1
#define STATE_FLAG_FRAMEADDED 0x2
#define STATE_FLAG_NEW_INPUT  0x4

struct eval_frame {
	node_t *state_hdl;
	node_t *restart_hdl;
	node_t *cursor_hdl;
	node_t *newargs_last_hdl;
	node_t *newargs_hdl;
	node_t *env_hdl_hdl;
	node_t *func_hdl;
	node_t *in_hdl;
};

static value_t locals_state_get(struct eval_frame *f)
{
	if(node_handle(f->state_hdl)) {
		return node_value(node_handle(f->state_hdl));
	} else {
		return 0;
	}
}

static void locals_state_set(struct eval_frame *f, value_t v)
{
	node_handle_update(f->state_hdl, node_value_new(v));
}

static void *locals_restart_get(struct eval_frame *f)
{
	if(node_handle(f->restart_hdl)) {
		return node_blob_addr(node_handle(f->restart_hdl));
	} else {
		return NULL;
	}
}

static void locals_restart_set(struct eval_frame *f, void *addr)
{
	node_handle_update(f->restart_hdl, node_blob_new(addr, NULL));
}

static node_t *locals_cursor_get(struct eval_frame *f)
{
	return node_handle(f->cursor_hdl);
}

static void locals_cursor_set(struct eval_frame *f, node_t *curs)
{
	node_handle_update(f->cursor_hdl, curs);
}

static node_t *locals_newargs_last_get(struct eval_frame *f)
{
	return node_handle(f->newargs_last_hdl);
}

static void locals_newargs_last_set(struct eval_frame *f, node_t *last)
{
	node_handle_update(f->newargs_last_hdl, last);
}

static node_t *locals_newargs_get(struct eval_frame *f)
{
	return node_handle(f->newargs_hdl);
}

static void locals_newargs_set(struct eval_frame *f, node_t *args)
{
	node_handle_update(f->newargs_hdl, args);
}

static node_t *locals_env_hdl_get(struct eval_frame *f)
{
	return node_handle(f->env_hdl_hdl);
}

static void locals_env_hdl_set(struct eval_frame *f, node_t *env)
{
	node_handle_update(f->env_hdl_hdl, env);
}

static node_t *locals_func_get(struct eval_frame *f)
{
	return node_handle(f->func_hdl);
}

static void locals_func_set(struct eval_frame *f, node_t *func)
{
	node_handle_update(f->func_hdl, func);
}

static node_t *locals_in_get(struct eval_frame *f)
{
	return node_handle(f->in_hdl);
}

static void locals_in_set(struct eval_frame *f, node_t *in)
{
	node_handle_update(f->in_hdl, in);
}

/* frame format:

   frame_hdl
   +---+
   |   |
   +---+
     |
     |
     v
   top frame    (parent frame)
   +---+---+    +---+---+
   |   | ------>|   | -----> ...
   +-|-+---+    +-|-+---+
     |            v
     |            ...
     |
     v
   entry        (next entry)
   +---+---+    +---+---+
   |   | ------>|   | -----> ...
   +-|-+---+    +-|-+---+
     |            |
     v            v
   val_hdl
   +---+
   |   |
   +-|-+
     |
     v
    val 
   +---+
   | ? |
   +---+
*/

void frame_push(
	node_t *frame_hdl,
	struct eval_frame *locals,
	node_t *in,
	node_t *env_handle)
{
	node_t *newframe, *oldframe;

	assert(node_type(frame_hdl) == NODE_HANDLE);
	assert(locals);
	assert(node_type(env_handle) == NODE_HANDLE);

	oldframe = node_handle(frame_hdl);
	newframe = NULL;

	newframe = node_cons_new(node_handle(locals->state_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->restart_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->cursor_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->newargs_last_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->newargs_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->env_hdl_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->func_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->in_hdl), newframe);

	newframe = node_cons_new(newframe, oldframe);
	node_handle_update(frame_hdl, newframe);

	node_handle_update(locals->state_hdl, NULL);
	node_handle_update(locals->restart_hdl, NULL);
	node_handle_update(locals->cursor_hdl, NULL);
	node_handle_update(locals->newargs_last_hdl, NULL);
	node_handle_update(locals->newargs_hdl, NULL);
	node_handle_update(locals->env_hdl_hdl, env_handle);
	node_handle_update(locals->func_hdl, NULL);
	node_handle_update(locals->in_hdl, in);
}

void frame_pop(
	node_t *frame_hdl,
	struct eval_frame *locals)
{
	node_t *newframe, *cursor;

	assert(node_type(frame_hdl) == NODE_HANDLE);
	assert(node_handle(frame_hdl));
	assert(locals);

	newframe = node_cons_cdr(node_handle(frame_hdl));
	cursor = node_cons_car(node_handle(frame_hdl));
	assert(cursor);

	node_handle_update(locals->in_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->func_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->env_hdl_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->newargs_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->newargs_last_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->cursor_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->restart_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->state_hdl, node_cons_car(cursor));

	node_handle_update(frame_hdl, newframe);
}

void frame_restore(
	node_t *frame_hdl,
	struct eval_frame *locals,
	node_t *snapshot)
{
	/* push an empty dummy frame, then reuse frame_pop */	
	node_t *dummy = node_cons_new(NULL, snapshot);
	node_handle_update(frame_hdl, dummy);
	frame_pop(frame_hdl, locals);
}

void frame_print_bt(node_t *frame_hdl)
{
	node_t *frame_curs, *var_curs, *val;

	for(frame_curs = node_handle(frame_hdl);
	    frame_curs;
	    frame_curs = node_cons_cdr(frame_curs)) {

		assert(node_type(frame_curs) == NODE_CONS);
		printf("frame @ %p\n", frame_curs);

		for(var_curs = node_cons_car(frame_curs);
		    var_curs;
		    var_curs = node_cons_cdr(var_curs)) {

			assert(node_type(var_curs) == NODE_CONS);
			val = node_cons_car(var_curs);
			printf("val @ %p ", val);
			node_print(val);
		}
		printf("\n");
	}
}

eval_err_t eval(node_t *in_handle, node_t *env_handle, node_t *out_handle)
{
	eval_err_t status = EVAL_OK;

	/* saved local variables */
	struct eval_frame locals;
	#define _STATE locals_state_get(&locals)
	#define _RESTART locals_restart_get(&locals)
	#define _CURSOR locals_cursor_get(&locals)
	#define _NEWARGS_LAST locals_newargs_last_get(&locals)
	#define _NEWARGS locals_newargs_get(&locals)
	#define _ENV_HDL locals_env_hdl_get(&locals)
	#define _FUNC locals_func_get(&locals)
	#define _INPUT locals_in_get(&locals)
	#define _SET_STATE(x) locals_state_set(&locals, (x))
	#define _SET_RESTART(x) locals_restart_set(&locals, (x))
	#define _SET_CURSOR(x) locals_cursor_set(&locals, (x))
	#define _SET_NEWARGS_LAST(x) locals_newargs_last_set(&locals, (x))
	#define _SET_NEWARGS(x) locals_newargs_set(&locals, (x))
	#define _SET_ENV_HDL(x) locals_env_hdl_set(&locals, (x))
	#define _SET_FUNC(x) locals_func_set(&locals, (x))
	#define _SET_INPUT(x) locals_in_set(&locals, (x))

	/* long-lived local variables */
	node_t *frame_hdl;
	node_t *result_handle;
	size_t bt_depth = 0;
	size_t env_depth = 0;

	/* temps -- must be restored after recursive calls */
	node_t *_args;
	node_t *keyval;
	node_t *temp;

	locals.state_hdl = node_lockroot(node_handle_new(NULL));
	locals.restart_hdl = node_lockroot(node_handle_new(NULL));
	locals.cursor_hdl = node_lockroot(node_handle_new(NULL));
	locals.newargs_last_hdl = node_lockroot(node_handle_new(NULL));
	locals.newargs_hdl = node_lockroot(node_handle_new(NULL));
	locals.env_hdl_hdl = node_lockroot(node_handle_new(NULL));
	locals.func_hdl = node_lockroot(node_handle_new(NULL));
	locals.in_hdl = node_lockroot(node_handle_new(NULL));

	assert(node_type(in_handle) == NODE_HANDLE);
	assert(node_type(env_handle) == NODE_HANDLE);
	assert(node_type(out_handle) == NODE_HANDLE);

	frame_hdl = node_lockroot(node_handle_new(NULL));
	result_handle = node_lockroot(node_handle_new(NULL));

	_SET_INPUT(node_handle(in_handle));
	_SET_ENV_HDL(env_handle);

restart:
#if defined(EVAL_TRACING)
	printf("%zu eval enter %p: -> ", bt_depth, _INPUT);
	node_print_pretty(_INPUT, false);
	printf("\n");
#endif

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
		if(! environ_lookup(node_handle(_ENV_HDL),
		                    _INPUT,
		                    &temp)) {
			node_handle_update(result_handle, _INPUT);
			status = eval_err(EVAL_ERR_UNRESOLVED_SYMBOL);
		}
		node_handle_update(result_handle, temp);
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
	bt_depth++;
	frame_push(frame_hdl, &locals,
	           node_cons_car(_INPUT),
	           _ENV_HDL);
	goto restart;
	node_cons_post_eval_func:
	if(status != EVAL_OK) {
		goto node_cons_cleanup;
	}
	_SET_FUNC(node_handle(result_handle));

	if(node_type(_FUNC) == NODE_SPECIAL_FUNC) {
		switch(node_special_func(_FUNC)) {
		case SPECIAL_IF:
			/* (if args),  args -> (test pass fail) */
			/* status = eval(test, locals.env_handle, &result); */
			_SET_RESTART(&&node_if_post_eval_test);
			bt_depth++;
			frame_push(frame_hdl, &locals,
			           node_cons_car(_args), _ENV_HDL);
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
			temp = node_lambda_new(node_handle(_ENV_HDL),
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
			temp = node_cons_new(node_cont_new(node_handle(frame_hdl)),
			                     NULL);
			temp = node_cons_new(node_cons_car(_args), temp);
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
			bt_depth++;
			frame_push(frame_hdl, &locals,
			           node_cons_car(node_cons_cdr(_args)),
			           _ENV_HDL);
			goto restart;
			node_special_def_set_post_eval_func:
			if(status != EVAL_OK) {
				goto node_cons_cleanup;
			}
		
			if(node_special_func(_FUNC) == SPECIAL_DEF) {
				temp = node_handle(result_handle);
				environ_add(_ENV_HDL, _CURSOR, temp);
			} else {
				if(!environ_keyval(node_handle(_ENV_HDL),
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
				/* handled below */
				break;
		}
	}

	/* eval passed arguments in caller's environment */
	_SET_NEWARGS_LAST(NULL);
	_SET_NEWARGS(NULL);
	for(_SET_CURSOR(node_cons_cdr(_INPUT));
	    _CURSOR;
		_SET_CURSOR(node_cons_cdr(_CURSOR))) {

		_SET_RESTART(&&node_cons_post_args_eval);
		bt_depth++;
		frame_push(frame_hdl, &locals,
		                node_cons_car(_CURSOR),
		                _ENV_HDL);
		goto restart;
		node_cons_post_args_eval:
		if(status != EVAL_OK) {
			_SET_NEWARGS_LAST(NULL);
			_SET_NEWARGS(NULL);
			goto node_lambda_cleanup;
		}

		temp = node_handle(result_handle);
		if(_NEWARGS == NULL) {
			_SET_NEWARGS(node_cons_new(temp, NULL));
			_SET_NEWARGS_LAST(_NEWARGS);
		} else {
			node_cons_patch_cdr(_NEWARGS_LAST,
			                    node_cons_new(temp, NULL));
			_SET_NEWARGS_LAST(node_cons_cdr(_NEWARGS_LAST));
		}
	}

	switch(node_type(_FUNC)) {
	case NODE_SPECIAL_FUNC:
		switch(node_special_func(_FUNC)) {
		case SPECIAL_DEFINED:
			if(_NEWARGS) {
				temp = node_cons_car(_NEWARGS);
				if(node_type(temp) == NODE_SYMBOL
				   && environ_lookup(node_handle(_ENV_HDL),
				                     temp, NULL)) {
					node_handle_update(result_handle, node_value_new(1));
				}
			}
			goto node_cons_cleanup;
		default:
			assert(false);
		}
	case NODE_FOREIGN:
		status = node_foreign_func(_FUNC)(_NEWARGS,
		                                  _ENV_HDL,
		                                  &temp);
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
			node_t *newhandle = node_handle_new(lambda_env);
			_SET_ENV_HDL(newhandle);
			environ_pushframe(_ENV_HDL);
			env_depth++;
#if defined(EVAL_TRACING)
		printf("%zu eval env_depth++ -> %zu\n", bt_depth, env_depth);
#endif
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
				_SET_NEWARGS_LAST(NULL);
				_SET_NEWARGS(NULL);
				_SET_FUNC(NULL);
				/* tailcall */
				goto restart;
			} else {
				/* status = eval(node_cons_car(cursor), env, [ignored]); */
				_SET_RESTART(&&node_lambda_post_exprs_eval);
				bt_depth++;
				frame_push(frame_hdl, &locals,
				           node_cons_car(_CURSOR),
				           _ENV_HDL);
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
	_SET_NEWARGS_LAST(NULL);

finish:
	if(_STATE & STATE_FLAG_FRAMEADDED) {
		environ_popframe(_ENV_HDL);
		env_depth--;
#if defined(EVAL_TRACING)
		printf("%zu eval env_depth-- -> %zu\n", bt_depth, env_depth);
#endif
		_SET_ENV_HDL(NULL);
	}


	if(node_handle(frame_hdl)) {
		_SET_NEWARGS(NULL);
		_SET_NEWARGS_LAST(NULL);
		if(_STATE & STATE_FLAG_NEW_INPUT) {
			_SET_INPUT(NULL);
		}
#if defined(EVAL_TRACING)
		printf("%zu eval leave %p: <- ", bt_depth, _INPUT);
		node_print_pretty(_INPUT, false);
		printf(" ... ");
		node_print_pretty(node_handle(result_handle), false);
		printf("\n");
#endif
		frame_pop(frame_hdl, &locals);
		bt_depth--;
		assert(_RESTART);
		goto *_RESTART;
	}


	node_handle_update(out_handle, node_handle(result_handle));
	node_droproot(frame_hdl);
	node_droproot(result_handle);

	node_droproot(locals.state_hdl);
	node_droproot(locals.restart_hdl);
	node_droproot(locals.cursor_hdl);
	node_droproot(locals.newargs_last_hdl);
	node_droproot(locals.newargs_hdl);
	node_droproot(locals.env_hdl_hdl);
	node_droproot(locals.func_hdl);
	node_droproot(locals.in_hdl);

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
	#undef _SET_NEWARGS_LAST
	#undef _SET_NEWARGS
	#undef _SET_ENV_HDL
	#undef _SET_FUNC
	#undef _SET_INPUT
}
