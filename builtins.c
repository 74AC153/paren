#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <alloca.h>

#include "environ.h"
#include "builtins.h"
#include "eval.h"
#include "foreign_common.h"

static
eval_err_t nil_matchfn(memory_state_t *ms, node_t **in, node_t **out, void *p)
{
	(void) p;
	if(node_type(in[0]) == NODE_NIL) {
		*out = node_value_new(ms, 1);
	} else {
		*out = NULL;
	}
	return EVAL_OK;
}
eval_err_t foreign_nil_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 1, nil_matchfn, args, out, NULL);
}

static
eval_err_t value_matchfn(memory_state_t *ms, node_t **in, node_t **out, void *p)
{
	(void) p;
	if(node_type(in[0]) == NODE_VALUE) {
		*out = node_value_new(ms, 1);
	} else {
		*out = NULL;
	}
	return EVAL_OK;
}
eval_err_t foreign_val_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 1, value_matchfn, args, out, NULL);
}

static
eval_err_t sym_matchfn(memory_state_t *ms, node_t **in, node_t **out, void *p)
{
	(void) p;
	if(node_type(in[0]) == NODE_SYMBOL) {
		*out = node_value_new(ms, 1);
	} else {
		*out = NULL;
	}
	return EVAL_OK;
}
eval_err_t foreign_sym_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 1, sym_matchfn, args, out, NULL);
}

static
eval_err_t cons_matchfn(memory_state_t *ms, node_t **in, node_t **out, void *p)
{
	(void) p;
	if(node_type(in[0]) == NODE_SYMBOL) {
		*out = node_value_new(ms, 1);
	} else {
		*out = NULL;
	}
	return EVAL_OK;
}
eval_err_t foreign_cons_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 1, cons_matchfn, args, out, NULL);
}

static
eval_err_t func_matchfn(memory_state_t *ms, node_t **in, node_t **out, void *p)
{
	nodetype_t type = node_type(in[0]);
	(void) p;
	if(type == NODE_LAMBDA
	   || type == NODE_FOREIGN
	   || type == NODE_CONTINUATION
	   || type == NODE_SPECIAL_FUNC) {
		*out = node_value_new(ms, 1);
	} else {
		*out = NULL;
	}
	return EVAL_OK;
}
eval_err_t foreign_func_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 1, func_matchfn, args, out, NULL);
}

static
eval_err_t do_car(memory_state_t *ms, node_t **in, node_t **out, void *p)
{
	(void) p;
	if(node_type(in[0]) != NODE_CONS) {
		*out = in[0];
		return eval_err(EVAL_ERR_EXPECTED_CONS);
	}
	*out = node_cons_car(in[0]);
	return EVAL_OK;
}

eval_err_t foreign_car(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 1, do_car, args, out, NULL);
}

static
eval_err_t do_cdr(memory_state_t *ms, node_t **args, node_t **out, void *p)
{
	(void) p;
	if(node_type(args[0]) != NODE_CONS) {
		*out = args[0];
		return eval_err(EVAL_ERR_EXPECTED_CONS);
	}
	*out = node_cons_cdr(args[0]);
	return EVAL_OK;
}

eval_err_t foreign_cdr(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 1, do_cdr, args, out, NULL);
}

static
eval_err_t do_makesym(memory_state_t *ms, node_t **args, node_t **out, void *p)
{
	char name[MAX_SYM_LEN], *cursor;;
	node_t *val, *val_iter;

	(void) p;
	val = args[0];
	if(node_type(val) != NODE_CONS) {
		*out = val;
		return eval_err(EVAL_ERR_EXPECTED_CONS);
	}
	
	val_iter = val;
	cursor = name;
	for(val_iter = val; val_iter; val_iter = node_cons_cdr(val_iter)) {
		val = node_cons_car(val_iter);
		if(node_type(val) != NODE_VALUE) {
			*out = val;
			return eval_err(EVAL_ERR_EXPECTED_VALUE);
		}
		if(node_value(val) > 255 ) {
			*out = val;
			return eval_err(EVAL_ERR_VALUE_BOUNDS);
		}
		*cursor++ = node_value(val);
		if(cursor - &(name[0]) >= (ssize_t) sizeof(name)) {
			break;
		}
		val_iter = node_cons_cdr(val_iter);
	}
	*cursor = 0;
	*out = node_symbol_new(ms, name);

	return EVAL_OK;
}
eval_err_t foreign_mksym(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 1, do_makesym, args, out, NULL);
}


static
eval_err_t do_splitsym(memory_state_t *ms, node_t **args, node_t **out, void *p)
{
	char *start, *end;
	node_t *sym;

	(void) p;
	sym = args[0];
	if(node_type(sym) != NODE_SYMBOL) {
		*out = sym;
		return eval_err(EVAL_ERR_EXPECTED_SYMBOL);
	}

	start = node_symbol_name(sym);
	end = start + strlen(start) - 1; // end points to last nonzero char

	/* construct value list backwards */
	for(*out = NULL; end >= start; end--) {
		*out = node_cons_new(ms,
		                     node_value_new(ms, *end),
		                     *out);
	}

	return EVAL_OK;
}
eval_err_t foreign_splsym(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 1, do_splitsym, args, out, NULL);
}

static
eval_err_t do_cons(memory_state_t *ms, node_t **args, node_t **out, void *p)
{
	(void) p;
	*out = node_cons_new(ms, args[0], args[1]);
	return EVAL_OK;
}
eval_err_t foreign_cons(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_cons, args, out, NULL);
}

static
eval_err_t do_symeq_p(memory_state_t *ms, node_t **args, node_t **out, void *p)
{
	(void) p;
	if(node_type(args[0]) != NODE_SYMBOL) {
		*out = args[0];
		return eval_err(EVAL_ERR_EXPECTED_SYMBOL);
	}
	if(node_type(args[1]) != NODE_SYMBOL) {
		*out = args[1];
		return eval_err(EVAL_ERR_EXPECTED_SYMBOL);
	}
	
	*out = NULL;
	if(strcmp(node_symbol_name(args[0]), node_symbol_name(args[1]))) {
		*out = node_value_new(ms, 1);
	}
	return EVAL_OK;
}
eval_err_t foreign_smeq_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_symeq_p, args, out, NULL);
}

typedef void (*mathop_t)(memory_state_t *ms, value_t a, value_t b, node_t **out);

static
eval_err_t do_mathop(memory_state_t *ms, node_t **args, node_t **out, void *p)
{
	mathop_t fn = (mathop_t) p;

	if(node_type(args[0]) != NODE_VALUE) {
		*out = args[0];
		return eval_err(EVAL_ERR_EXPECTED_VALUE);
	}
	if(node_type(args[1]) != NODE_VALUE) {
		*out = args[1];
		return eval_err(EVAL_ERR_EXPECTED_VALUE);
	}
	fn(ms, node_value(args[0]), node_value(args[1]), out);
	return EVAL_OK;

}

static
void mathop_eq_p(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = NULL;
	if(a == b) *out = node_value_new(ms, 1);
}
eval_err_t foreign_eq_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_eq_p);
}

static
void mathop_lt_p(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = NULL;
	if(a < b) *out = node_value_new(ms, 1);
}
eval_err_t foreign_lt_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_lt_p);
}

static
void mathop_gt_p(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = NULL;
	if(a > b) *out = node_value_new(ms, 1);
}
eval_err_t foreign_gt_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_gt_p);
}

static
void mathop_ult_p(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = NULL;
	if((u_value_t) a < (u_value_t) b) *out = node_value_new(ms, 1);
}
eval_err_t foreign_ult_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_ult_p);
}

static
void mathop_ugt_p(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = NULL;
	if((u_value_t) a > (u_value_t) b) *out = node_value_new(ms, 1);
}
eval_err_t foreign_ugt_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_ugt_p);
}

static
void mathop_addc_p(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	#define ALLBITS ( ~((value_t)0) )
	#define HIGHBIT ( (u_value_t) ALLBITS ^ ((u_value_t) ALLBITS >> 1) )

	*out = NULL;
	if((a & HIGHBIT) && (b & HIGHBIT)) *out = node_value_new(ms, 1);

	#undef HIGHBIT
	#undef ALLBITS
}
eval_err_t foreign_addc_p(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_addc_p);
}

static
void mathop_add(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, a + b);
}
eval_err_t foreign_add(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_add);
}

static
void mathop_sub(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, a - b);
}
eval_err_t foreign_sub(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_sub);
}

static
void mathop_mul(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, a * b);
}
eval_err_t foreign_mul(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_mul);
}

static
void mathop_div(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out =node_value_new(ms, a / b);
}
eval_err_t foreign_div(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_div);
}

static
void mathop_rem(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, a % b);
}
eval_err_t foreign_rem(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_rem);
}

static
void mathop_bit_and(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, a & b);
}
eval_err_t foreign_b_and(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_bit_and);
}

static
void mathop_bit_or(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, a | b);
}
eval_err_t foreign_b_or(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_bit_or);
}

static
void mathop_bit_nand(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, ~(a & b));
}
eval_err_t foreign_b_nand(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_bit_nand);
}

static
void mathop_bit_xor(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, a ^ b);
}
eval_err_t foreign_b_xor(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_bit_xor);
}

static
void mathop_bit_shl(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, a << b);
}
eval_err_t foreign_b_shl(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_bit_shl);
}

static
void mathop_bit_shr(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, (u_value_t) a >> b);
}
eval_err_t foreign_b_shr(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_bit_shr);
}

static
void mathop_bit_shra(memory_state_t *ms, value_t a, value_t b, node_t **out)
{
	*out = node_value_new(ms, a >> b);
}
eval_err_t foreign_b_shra(memory_state_t *ms, node_t *args, node_t *env, node_t **out)
{
	return extract_args(ms, 2, do_mathop, args, out, mathop_bit_shra);
}
