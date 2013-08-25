#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <stdbool.h>
#include "token.h"
#include "eval_err.h"

#define NODE_TYPES \
X(NODE_UNINITIALIZED) \
X(NODE_NIL) \
X(NODE_CONS) \
X(NODE_LAMBDA) \
X(NODE_SYMBOL) \
X(NODE_VALUE) \
X(NODE_FOREIGN) \
X(NODE_HANDLE) \
X(NODE_CONTINUATION) \
X(NODE_SPECIAL_FUNC) \
X(NODE_BLOB)

typedef enum {
#define X(name) name,
NODE_TYPES
#undef X
} nodetype_t;

#define NODE_SPECIALS \
X(SPECIAL_IF)\
X(SPECIAL_LAMBDA)\
X(SPECIAL_QUOTE)\
X(SPECIAL_MK_CONT)\
X(SPECIAL_DEF)\
X(SPECIAL_SET)\
X(SPECIAL_DEFINED) \
X(SPECIAL_EVAL)

typedef enum {
#define X(name) name,
NODE_SPECIALS
#undef X
} special_func_t;

extern char *node_type_names[];
extern char *node_special_names[];

struct node;
typedef struct node node_t;

typedef eval_err_t (*foreign_t)(node_t *args,
                                node_t *env_handle,
                                node_t **result);
typedef int64_t value_t;
typedef uint64_t u_value_t;

typedef void (*blob_fin_t)(void *addr);

struct node {
	nodetype_t type;
	union {
		struct { node_t *car, *cdr; } cons;
		struct { node_t *env, *vars, *expr; } lambda;
		foreign_t func;
		char name[MAX_SYM_LEN];
		value_t value;
		struct { node_t *link; } handle;
		struct { node_t *bt; } cont;
		struct { void *addr; blob_fin_t fin; uintptr_t sig; } blob;
		special_func_t special;
	} dat;
};

void nodes_initialize();
void nodes_reset();

nodetype_t node_type(node_t *n);
/* (locked, root), (unlocked, root), (unlocked, not root) */
/* NB: (unlocked, root) is only initial state for new nodes */
void node_droproot(node_t *n); /* -> (unlocked, not root) */
bool node_isroot(node_t *n);
node_t *node_lockroot(node_t *n); /* -> (locked, root) */
bool node_islocked(node_t *n);

node_t *node_cons_new(node_t *car, node_t *cdr);
node_t *node_cons_car(node_t *n);
node_t *node_cons_cdr(node_t *n);
void node_cons_patch_car(node_t *n, node_t *newcar);
void node_cons_patch_cdr(node_t *n, node_t *newcdr);

node_t *node_lambda_new(node_t *env, node_t *vars, node_t *expr);
node_t *node_lambda_env(node_t *n);
node_t *node_lambda_vars(node_t *n);
node_t *node_lambda_expr(node_t *n);

node_t *node_value_new(value_t val);
value_t node_value(node_t *n);

node_t *node_symbol_new(char *name);
char *node_symbol_name(node_t *n);

node_t *node_foreign_new(foreign_t func);
foreign_t node_foreign_func(node_t *n);

node_t *node_quote_new(node_t *val);
node_t *node_quote_val(node_t *n);

node_t *node_handle_new(node_t *link);
node_t *node_handle(node_t *n);
void node_handle_update(node_t *n, node_t *newlink);

node_t *node_cont_new(node_t *bt);
node_t *node_cont(node_t *n);

node_t *node_special_func_new(special_func_t func);
special_func_t node_special_func(node_t *n);

node_t *node_blob_new(void *addr, blob_fin_t fin, uintptr_t sig);
void *node_blob_addr(node_t *n);

void node_print(node_t *n);
void node_print_recursive(node_t *n);
void node_print_pretty(node_t *n, bool verbose);

/* run garbage collector one full cycle */
void node_gc(void);
void node_gc_print_state(void);
void node_gc_statistics(
	uintptr_t *total_alloc,
	uintptr_t *total_free,
	unsigned long long *iter_count,
	unsigned long long *cycle_count);

#endif
