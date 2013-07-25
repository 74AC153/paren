#if ! defined(BUILTIN_LOAD_H)
#define BUILTIN_LOAD_H

#include "node.h"
#include "eval_err.h"

eval_err_t foreign_load(node_t *args, node_t *env_handle, node_t **result);

#endif
