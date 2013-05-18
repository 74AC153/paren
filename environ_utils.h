#ifndef ENVIRON_UTILS_H
#define ENVIRON_UTILS_H

#include <stdlib.h>
#include <stdbool.h>
#include "node.h"
#include "environ.h"

typedef struct {
	char *name;
	node_t *(*func)(void);
} foreign_assoc_t;

void environ_add_builtins(node_t **env,
                          foreign_assoc_t *builtins,
                          size_t num);


#endif
