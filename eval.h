#ifndef EVAL_H
#define EVAL_H

#include "node.h"
#include "eval_err.h"

eval_err_t eval(node_t *in, node_t *env_handle, node_t **out);

#endif
