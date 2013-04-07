#include "eval_err.h"

static char *eval_err_messages[] = {
#define X(A, B) B,
EVAL_ERR_DEFS
#undef X
};

char *eval_err_str(eval_err_t err)
{
	return eval_err_messages[err];
}
