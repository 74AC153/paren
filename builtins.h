#ifndef BUILTINS_H
#define BUILTINS_H

#include "node.h"
#include "eval_err.h"

eval_err_t foreign_quote(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_defbang(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_setbang(node_t *args, node_t *env_handle, node_t **result);

eval_err_t foreign_atom(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_car(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_cdr(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_cons(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_eq(node_t *args, node_t *env_handle, node_t **result);

eval_err_t foreign_makesym(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_splitsym(node_t *args, node_t *env_handle, node_t **result);

#endif
