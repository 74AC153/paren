#if ! defined(BUILTIN_LOAD_H)
#define BUILTIN_LOAD_H

#include "node.h"
#include "eval_err.h"

eval_err_t foreign_loadlib(memory_state_t *ms, node_t *args, node_t *env_handle, node_t **result);

eval_err_t foreign_read_eval(memory_state_t *ms, node_t *args, node_t *env_handle, node_t **result);

#endif
