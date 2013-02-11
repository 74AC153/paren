#ifndef EVAL_H
#define EVAL_H

#include <stdbool.h>
#include "node.h"

#define EVAL_ERR_DEFS \
X(EVAL_OK, "no error") \
X(EVAL_ERR_UNRESOLVED_SYMBOL, "unresolved symbol") \
X(EVAL_ERR_TOO_MANY_ARGS, "too many arguments to function") \
X(EVAL_ERR_MISSING_ARG, "missing argument") \
X(EVAL_ERR_FUNC_IS_NOT_LAMBDA, "expected lambda") \
X(EVAL_ERR_NONLAMBDA_FUNCALL, "function call with non lambda symbol") \
X(EVAL_ERR_UNKNOWN_FUNCALL, "invalid function call") \
X(EVAL_ERR_EXPECTED_LIST, "expected list") \
X(EVAL_ERR_EXPECTED_SYMBOL, "expected symbol")

typedef enum {
#define X(A, B) A,
EVAL_ERR_DEFS
#undef X
} eval_err_t;

eval_err_t eval(node_t *input, node_t **environ, node_t **output);

char *eval_err_str(eval_err_t err);

#endif
