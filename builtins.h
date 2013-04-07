#ifndef BUILTINS_H
#define BUILTINS_H

#include "node.h"
#include "eval_err.h"

eval_err_t builtin_quote(node_t *args, node_t **env, node_t **result);
eval_err_t builtin_defbang(node_t *args, node_t **env, node_t **result);
eval_err_t builtin_setbang(node_t *args, node_t **env, node_t **result);

eval_err_t builtin_atom(node_t *args, node_t **env, node_t **result);
eval_err_t builtin_car(node_t *args, node_t **env, node_t **result);
eval_err_t builtin_cdr(node_t *args, node_t **env, node_t **result);
eval_err_t builtin_if(node_t *args, node_t **env, node_t **result);
eval_err_t builtin_cons(node_t *args, node_t **env, node_t **result);
eval_err_t builtin_eq(node_t *args, node_t **env, node_t **result);

eval_err_t builtin_lambda(node_t *args, node_t **env, node_t **result);

#endif
