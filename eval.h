#ifndef EVAL_H
#define EVAL_H

#include <stdbool.h>
#include "node.h"
#include "eval_err.h"

eval_err_t eval(node_t *input, node_t **environ, node_t **output);

#endif
