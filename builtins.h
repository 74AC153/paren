#ifndef BUILTINS_H
#define BUILTINS_H

#include "node.h"
#include "eval_err.h"

/* node -> bool */
eval_err_t foreign_nil_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_val_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_sym_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_cons_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_func_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);

/* node -> node */
eval_err_t foreign_car(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_cdr(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_mksym(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_splsym(memory_state_t *ms, node_t *in, node_t *env, node_t **out);

/* node, node -> node */
eval_err_t foreign_cons(memory_state_t *ms, node_t *in, node_t *env, node_t **out);

eval_err_t foreign_smeq_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);

/* mathops: node, node -> node */
eval_err_t foreign_eq_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_lt_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_gt_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_ult_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_ugt_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);

eval_err_t foreign_addc_p(memory_state_t *ms, node_t *in, node_t *env, node_t **out);

eval_err_t foreign_add(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_sub(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_mul(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_div(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_rem(memory_state_t *ms, node_t *in, node_t *env, node_t **out);

eval_err_t foreign_b_and(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_b_or(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_b_nand(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_b_xor(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_b_shl(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_b_shr(memory_state_t *ms, node_t *in, node_t *env, node_t **out);
eval_err_t foreign_b_shra(memory_state_t *ms, node_t *in, node_t *env, node_t **out);

/* TODO */

#endif
