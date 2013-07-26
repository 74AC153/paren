#include <alloca.h>
#include "foreign_common.h"
#include "eval_err.h"

eval_err_t extract_args(
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

