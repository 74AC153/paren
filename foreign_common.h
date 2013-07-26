#if ! defined(FOREIGN_COMMON_H)
#define FOREIGN_COMMON_H

#include "node.h"

typedef eval_err_t (*callback_t)(node_t **args, node_t **result, void *p);

eval_err_t extract_args(
	unsigned int n,
	callback_t cb,
	node_t *args,
	node_t **result,
	void *p);

#endif
