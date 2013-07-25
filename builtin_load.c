#include <assert.h>
#include <alloca.h>
#include <stdlib.h>

#include "builtin_load.h"
#include "node.h"
#include "environ.h"
#include "load_wrapper.h"

typedef eval_err_t (*callback_t)(node_t **args, node_t **result, void *p);

static eval_err_t extract_args(
	unsigned int n,
	callback_t cb,
	node_t *args,
	node_t **result,
	void *p)
{
	unsigned int i;
	node_t *cursor = args;
	node_t **arglist = alloca(n * sizeof(node_t *));
	for(i = 0; i < n; i++) {
		if(node_type(cursor) != NODE_CONS) {
			*result = args;
			return eval_err(EVAL_ERR_EXPECTED_CONS);
		}
		arglist[i] = node_cons_car(cursor);
		cursor = node_cons_cdr(cursor);
	}
	if(cursor) {
		*result = args;
		return eval_err(EVAL_ERR_TOO_MANY_ARGS);
	}
	return cb(arglist, result, p);
}

static eval_err_t loadfn(node_t **n, node_t **result, void *p)
{
	char *path = NULL;
	int load_status;
	size_t len;
	ldwrap_ent_t *ld_funs = NULL, *ld_funs_cursor;
	eval_err_t status = EVAL_OK;
	node_t *cursor, *val;
	node_t *env_handle = (node_t *) p;

	/* count path length */
	for(cursor = n[0], len = 0;
	    cursor;
	    cursor = node_cons_cdr(cursor), len++) {
		if(node_type(cursor) != NODE_CONS) {
			status = EVAL_ERR_EXPECTED_CONS;
			goto cleanup;
		}
	}

	/* make path string */
	path = alloca(len + 1);
	if(!path) {
		status = EVAL_ERR_OUT_OF_MEM;
		goto cleanup;
	}
	
	for(cursor = n[0], len = 0; cursor; cursor = node_cons_cdr(cursor), len++) {
		if(node_type(cursor) != NODE_CONS) {
			status = EVAL_ERR_EXPECTED_CONS;
			goto cleanup;
		}
		val = node_cons_car(cursor);
		if(node_type(val) != NODE_VALUE) {
			status = EVAL_ERR_EXPECTED_VALUE;
			goto cleanup;
		}
		if(node_value(val) > 255 || node_value(val) < 0) {
			status = EVAL_ERR_VALUE_BOUNDS;
			goto cleanup;
		}
		path[len] = node_value(val);
	}
	path[len] = 0;

	/* load libs */
	load_status = load_wrapper(path, &ld_funs);
	
	/* update environment */
	assert(node_type(env_handle) == NODE_HANDLE);
	for(ld_funs_cursor = ld_funs; ld_funs_cursor->name; ld_funs_cursor++) {
		node_t *name, *val;
		name = node_symbol_new(ld_funs_cursor->name);
		val = node_foreign_new(ld_funs_cursor->fun);
		environ_add(env_handle, name, val);
	}

cleanup:
	free(ld_funs);
	return status;
}


eval_err_t foreign_load(node_t *args, node_t *env_handle, node_t **result)
{
	return extract_args(1, loadfn, args, result, env_handle);
}

