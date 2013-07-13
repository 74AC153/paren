#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <alloca.h>

#include "environ.h"
#include "builtins.h"
#include "eval.h"

typedef eval_err_t (*callback_t)(node_t **args, node_t **result, void *p);

static eval_err_t extract_args(
	unsigned int n,
	callback_t cb,
	node_t *args,
	node_t **result,
	void *p)
{
	unsigned int i;
	node_t *cursor = args;
	node_t **arglist = alloca(n * sizeof(node_t *));
	for(i = 0; i < n; i++) {
		if(node_type(cursor) != NODE_CONS) {
			*result = args;
			return eval_err(EVAL_ERR_EXPECTED_CONS);
		}
		arglist[i] = node_cons_car(cursor);
		cursor = node_cons_cdr(cursor);
	}
	if(cursor) {
		*result = args;
		return eval_err(EVAL_ERR_TOO_MANY_ARGS);
	}
	return cb(arglist, result, p);
}

static eval_err_t nil_matchfn(node_t **n, node_t **result, void *p)
{
	(void) p;
	*result = NULL;
	if(node_type(n[0]) == NODE_NIL)
		*result = node_value_new(1);
	return EVAL_OK;
}
eval_err_t foreign_nil_p(node_t *args, node_t **result)
{
	return extract_args(1, nil_matchfn, args, result, NULL);
}

static eval_err_t value_matchfn(node_t **n, node_t **result, void *p)
{
	(void) p;
	*result = NULL;
	if(node_type(n[0]) == NODE_VALUE)
		*result = node_value_new(1);
	return EVAL_OK;
}
eval_err_t foreign_value_p(node_t *args, node_t **result)
{
	return extract_args(1, value_matchfn, args, result, NULL);
}

static eval_err_t sym_matchfn(node_t **n, node_t **result, void *p)
{
	(void) p;
	*result = NULL;
	if(node_type(n[0]) == NODE_SYMBOL)
		*result = node_value_new(1);
	return EVAL_OK;
}
eval_err_t foreign_sym_p(node_t *args, node_t **result)
{
	return extract_args(1, sym_matchfn, args, result, NULL);
}

static eval_err_t cons_matchfn(node_t **n, node_t **result, void *p)
{
	(void) p;
	*result = NULL;
	if(node_type(n[0]) == NODE_SYMBOL)
		*result = node_value_new(1);
	return EVAL_OK;
}
eval_err_t foreign_cons_p(node_t *args, node_t **result)
{
	return extract_args(1, cons_matchfn, args, result, NULL);
}

static eval_err_t func_matchfn(node_t **n, node_t **result, void *p)
{
	nodetype_t type = node_type(n[0]);
	(void) p;
	*result = NULL;
	if(type == NODE_LAMBDA
	   || type == NODE_FOREIGN
	   || type == NODE_CONTINUATION
	   || type == NODE_SPECIAL_FUNC)
		*result = node_value_new(1);
	return EVAL_OK;
}
eval_err_t foreign_func_p(node_t *args, node_t **result)
{
	return extract_args(1, func_matchfn, args, result, NULL);
}

static eval_err_t do_car(node_t **args, node_t **result, void *p)
{
	(void) p;
	if(node_type(args[0]) != NODE_CONS) {
		*result = args[0];
		return eval_err(EVAL_ERR_EXPECTED_CONS);
	}
	*result = node_cons_car(args[0]);
	return EVAL_OK;
}

eval_err_t foreign_car(node_t *args, node_t **result)
{
	return extract_args(1, do_car, args, result, NULL);
}

static eval_err_t do_cdr(node_t **args, node_t **result, void *p)
{
	(void) p;
	if(node_type(args[0]) != NODE_CONS) {
		*result = args[0];
		return eval_err(EVAL_ERR_EXPECTED_CONS);
	}
	*result = node_cons_cdr(args[0]);
	return EVAL_OK;
}

eval_err_t foreign_cdr(node_t *args, node_t **result)
{
	return extract_args(1, do_cdr, args, result, NULL);
}

static eval_err_t do_makesym(node_t **args, node_t **result, void *p)
{
	char name[MAX_SYM_LEN], *cursor;;
	node_t *val, *val_iter;

	(void) p;
	val = args[0];
	if(node_type(val) != NODE_CONS) {
		*result = val;
		return eval_err(EVAL_ERR_EXPECTED_CONS);
	}
	
	val_iter = val;
	cursor = name;
	for(val_iter = val; val_iter; val_iter = node_cons_cdr(val_iter)) {
		val = node_cons_car(val_iter);
		if(node_type(val) != NODE_VALUE) {
			*result = val;
			return eval_err(EVAL_ERR_EXPECTED_VALUE);
		}
		if(node_value(val) > 255 ) {
			*result = val;
			return eval_err(EVAL_ERR_VALUE_BOUNDS);
		}
		*cursor++ = node_value(val);
		if(cursor - &(name[0]) >= (ssize_t) sizeof(name)) {
			break;
		}
		val_iter = node_cons_cdr(val_iter);
	}
	*cursor = 0;
	*result = node_symbol_new(name);

	return EVAL_OK;
}
eval_err_t foreign_makesym(node_t *args, node_t **result)
{
	return extract_args(1, do_makesym, args, result, NULL);
}


static eval_err_t do_splitsym(node_t **args, node_t **result, void *p)
{
	char *start, *end;
	node_t *sym;

	(void) p;
	sym = args[0];
	if(node_type(sym) != NODE_SYMBOL) {
		*result = sym;
		return eval_err(EVAL_ERR_EXPECTED_SYMBOL);
	}

	start = node_symbol_name(sym);
	end = start + strlen(start) - 1; // end points to last nonzero char

	/* construct value list backwards */
	for(*result = NULL; end >= start; end--) {
		*result = node_cons_new(node_value_new(*end), *result);
	}

	return EVAL_OK;
}
eval_err_t foreign_splitsym(node_t *args, node_t **result)
{
	return extract_args(1, do_splitsym, args, result, NULL);
}

static eval_err_t do_cons(node_t **args, node_t **result, void *p)
{
	(void) p;
	*result = node_cons_new(args[0], args[1]);
	return EVAL_OK;
}
eval_err_t foreign_cons(node_t *args, node_t **result)
{
	return extract_args(2, do_cons, args, result, NULL);
}

static eval_err_t do_symeq_p(node_t **args, node_t **result, void *p)
{
	(void) p;
	if(node_type(args[0]) != NODE_SYMBOL) {
		*result = args[0];
		return eval_err(EVAL_ERR_EXPECTED_SYMBOL);
	}
	if(node_type(args[1]) != NODE_SYMBOL) {
		*result = args[1];
		return eval_err(EVAL_ERR_EXPECTED_SYMBOL);
	}
	
	*result = NULL;
	if(strcmp(node_symbol_name(args[0]), node_symbol_name(args[1]))) {
		*result = node_value_new(1);
	}
	return EVAL_OK;
}
eval_err_t foreign_symeq_p(node_t *args, node_t **result)
{
	return extract_args(2, do_symeq_p, args, result, NULL);
}

typedef void (*mathop_t)(value_t a, value_t b, node_t **result);

static eval_err_t do_mathop(node_t **args, node_t **result, void *p)
{
	mathop_t fn = (mathop_t) p;

	if(node_type(args[0]) != NODE_VALUE) {
		*result = args[0];
		return eval_err(EVAL_ERR_EXPECTED_VALUE);
	}
	if(node_type(args[1]) != NODE_VALUE) {
		*result = args[1];
		return eval_err(EVAL_ERR_EXPECTED_VALUE);
	}
	fn(node_value(args[0]), node_value(args[1]), result);
	return EVAL_OK;

}

static void mathop_eq_p(value_t a, value_t b, node_t **result)
{
	*result = NULL;
	if(a == b) *result = node_value_new(1);
}
eval_err_t foreign_eq_p(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_eq_p);
}

static void mathop_lt_p(value_t a, value_t b, node_t **result)
{
	*result = NULL;
	if(a < b) *result = node_value_new(1);
}
eval_err_t foreign_lt_p(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_lt_p);
}

static void mathop_gt_p(value_t a, value_t b, node_t **result)
{
	*result = NULL;
	if(a > b) *result = node_value_new(1);
}
eval_err_t foreign_gt_p(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_gt_p);
}

static void mathop_ult_p(value_t a, value_t b, node_t **result)
{
	*result = NULL;
	if((u_value_t) a < (u_value_t) b) *result = node_value_new(1);
}
eval_err_t foreign_ult_p(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_ult_p);
}

static void mathop_ugt_p(value_t a, value_t b, node_t **result)
{
	*result = NULL;
	if((u_value_t) a > (u_value_t) b) *result = node_value_new(1);
}
eval_err_t foreign_ugt_p(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_ugt_p);
}

static void mathop_addc_p(value_t a, value_t b, node_t **result)
{
	#define ALLBITS ( ~((value_t)0) )
	#define HIGHBIT ( (u_value_t) ALLBITS ^ ((u_value_t) ALLBITS >> 1) )

	*result = NULL;
	if((a & HIGHBIT) && (b & HIGHBIT)) *result = node_value_new(1);

	#undef HIGHBIT
	#undef ALLBITS
}
eval_err_t foreign_addc_p(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_addc_p);
}

static void mathop_add(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(a + b);
}
eval_err_t foreign_add(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_add);
}

static void mathop_sub(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(a - b);
}
eval_err_t foreign_sub(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_sub);
}

static void mathop_mul(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(a * b);
}
eval_err_t foreign_mul(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_mul);
}

static void mathop_div(value_t a, value_t b, node_t **result)
{
	*result =node_value_new(a / b);
}
eval_err_t foreign_div(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_div);
}

static void mathop_rem(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(a % b);
}
eval_err_t foreign_rem(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_rem);
}

static void mathop_bit_and(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(a & b);
}
eval_err_t foreign_bit_and(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_bit_and);
}

static void mathop_bit_or(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(a | b);
}
eval_err_t foreign_bit_or(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_bit_or);
}

static void mathop_bit_nand(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(~(a & b));
}
eval_err_t foreign_bit_nand(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_bit_nand);
}

static void mathop_bit_xor(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(a ^ b);
}
eval_err_t foreign_bit_xor(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_bit_xor);
}

static void mathop_bit_shl(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(a << b);
}
eval_err_t foreign_bit_shl(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_bit_shl);
}

static void mathop_bit_shr(value_t a, value_t b, node_t **result)
{
	*result = node_value_new((u_value_t) a >> b);
}
eval_err_t foreign_bit_shr(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_bit_shr);
}

static void mathop_bit_shra(value_t a, value_t b, node_t **result)
{
	*result = node_value_new(a >> b);
}
eval_err_t foreign_bit_shra(node_t *args, node_t **result)
{
	return extract_args(2, do_mathop, args, result, mathop_bit_shra);
}
