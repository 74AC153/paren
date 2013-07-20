#ifndef BUILTINS_H
#define BUILTINS_H

#include "node.h"
#include "eval_err.h"

/* node -> bool */
eval_err_t foreign_nil_p(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_value_p(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_sym_p(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_cons_p(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_func_p(node_t *args, node_t *env_handle, node_t **result);

/* node -> node */
eval_err_t foreign_car(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_cdr(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_makesym(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_splitsym(node_t *args, node_t *env_handle, node_t **result);

/* node, node -> node */
eval_err_t foreign_cons(node_t *args, node_t *env_handle, node_t **result);

eval_err_t foreign_symeq_p(node_t *args, node_t *env_handle, node_t **result);

/* mathops: node, node -> node */
eval_err_t foreign_eq_p(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_lt_p(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_gt_p(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_ult_p(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_ugt_p(node_t *args, node_t *env_handle, node_t **result);

eval_err_t foreign_addc_p(node_t *args, node_t *env_handle, node_t **result);

eval_err_t foreign_add(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_sub(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_mul(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_div(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_rem(node_t *args, node_t *env_handle, node_t **result);

eval_err_t foreign_bit_and(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_bit_or(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_bit_nand(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_bit_xor(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_bit_shl(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_bit_shr(node_t *args, node_t *env_handle, node_t **result);
eval_err_t foreign_bit_shra(node_t *args, node_t *env_handle, node_t **result);

/* TODO */

#endif
