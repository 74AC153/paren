#ifndef BUILTINS_H
#define BUILTINS_H

#include "node.h"

int builtin_atom(node_t *args, node_t **env, node_t **result);
int builtin_car(node_t *args, node_t **env, node_t **result);
int builtin_cdr(node_t *args, node_t **env, node_t **result);
int builtin_cond(node_t *args, node_t **env, node_t **result);
int builtin_cons(node_t *args, node_t **env, node_t **result);
int builtin_eq(node_t *args, node_t **env, node_t **result);
int builtin_quote(node_t *args, node_t **env, node_t **result);
int builtin_label(node_t *args, node_t **env, node_t **result);

#endif
