#ifndef EVAL_H
#define EVAL_H

#include "node.h"
#include "eval_err.h"
#include "stream.h"

eval_err_t eval(
	memory_state_t *ms,
	node_t *env_handle,
	node_t *in_handle,
	node_t *out_handle);

#endif
