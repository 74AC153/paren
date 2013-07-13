#ifndef EVAL_ERR_H
#define EVAL_ERR_H

#define EVAL_ERR_DEFS \
X(EVAL_OK, "no error") \
X(EVAL_ERR_UNRESOLVED_SYMBOL, "unresolved symbol") \
X(EVAL_ERR_TOO_MANY_ARGS, "too many arguments to function") \
X(EVAL_ERR_TOO_FEW_ARGS, "too few arguments to function") \
X(EVAL_ERR_MISSING_ARG, "missing argument") \
X(EVAL_ERR_FUNC_IS_NOT_LAMBDA, "expected lambda") \
X(EVAL_ERR_NONLAMBDA_FUNCALL, "function with non lambda symbol") \
X(EVAL_ERR_UNKNOWN_FUNCALL, "invalid function call") \
X(EVAL_ERR_EXPECTED_CONS, "expected cons") \
X(EVAL_ERR_EXPECTED_SYMBOL, "expected symbol") \
X(EVAL_ERR_EXPECTED_VALUE, "expected value") \
X(EVAL_ERR_EXPECTED_CONS_SYM, "expected cons or symbol") \
X(EVAL_ERR_VALUE_BOUNDS, "given value out of bounds")

typedef enum {
#define X(A, B) A,
EVAL_ERR_DEFS
#undef X
} eval_err_t;

char *eval_err_str(eval_err_t err);

static eval_err_t eval_err(eval_err_t e) { return e; }

#endif
