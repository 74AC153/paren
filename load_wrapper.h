#if !defined(LOAD_WRAPPER_H)
#define LOAD_WRAPPER_H

#include "node.h"

typedef struct {
	char *name;
	void *fun;
} ldwrap_ent_t;

int load_wrapper(char *path, ldwrap_ent_t **funs);

static const char *load_wrapper_names_ident = "data_names";
static const char *load_wrapper_count_ident = "data_count";

#endif
