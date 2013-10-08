#ifndef ENVIRON_UTILS_H
#define ENVIRON_UTILS_H

#include <stdlib.h>
#include <stdbool.h>
#include "node.h"
#include "environ.h"
#include "memory.h"

typedef struct {
	char *name;
	node_t *(*func)(memory_state_t *s);
} foreign_assoc_t;

void environ_add_builtins(node_t *env_handle,
                          foreign_assoc_t *builtins,
                          size_t num);


#endif
