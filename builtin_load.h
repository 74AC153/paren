#if ! defined(BUILTIN_LOAD_H)
#define BUILTIN_LOAD_H

#include "node.h"
#include "eval_err.h"

eval_err_t foreign_loadlib(node_t *args, node_t *env_handle, node_t **result);

#define BLOB_SIG_MMAPFILE 0x6d6d6170 // 'mmap'
#if 0
eval_err_t foreign_mmapfile(node_t *args, node_t *env_handle, node_t **result);
#endif

#endif
