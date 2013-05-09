#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <stdbool.h>
#include "token.h"
#include "eval_err.h"

#define NODE_TYPES \
X(NODE_NIL) \
X(NODE_LIST) \
X(NODE_LAMBDA) \
X(NODE_SYMBOL) \
X(NODE_VALUE) \
X(NODE_BUILTIN) \
X(NODE_QUOTE) \
X(NODE_IF_FUNC) \
X(NODE_LAMBDA_FUNC) \
X(NODE_DEAD)

typedef enum {
#define X(name) name,
NODE_TYPES
#undef X
} nodetype_t;

extern char *node_type_names[];

struct node;
typedef struct node node_t;

typedef eval_err_t (*builtin_t)(struct node *args,
                                struct node **env,
                                struct node **result);

struct node {
	nodetype_t type;
	union {
		struct { struct node *child, *next; } list;
		struct { struct node *env, *vars, *expr; } lambda;
		builtin_t func;
		char name[MAX_SYM_LEN];
		uint64_t value;
		struct { struct node *val; } quote;
		struct { struct node *next; } dead;
	} dat;
};

void nodes_initialize();

nodetype_t node_type(node_t *n);
node_t *node_retain(node_t *n);
void node_release(node_t *n);
void node_retrel(node_t *n);
void node_forget(node_t *n);
void node_remember(node_t *n);
bool node_is_remembered(node_t *n);

node_t *node_new_list(node_t *c, node_t *n);
node_t *node_child_noref(node_t *n);
node_t *node_next_noref(node_t *n);
void node_patch_list_next(node_t *n, node_t *newnext);
void node_patch_list_child(node_t *n, node_t *newchld);

node_t *node_new_lambda(node_t *env, node_t *vars, node_t *expr);
node_t *node_lambda_env_noref(node_t *n);
node_t *node_lambda_vars_noref(node_t *n);
node_t *node_lambda_expr_noref(node_t *n);

node_t *node_new_value(uint64_t val);
uint64_t node_value(node_t *n);

node_t *node_new_symbol(char *name);
char *node_name(node_t *n);

node_t *node_new_builtin(builtin_t func);
builtin_t node_func(node_t *n);

node_t *node_new_quote(node_t *val);
node_t *node_quote_val_noref(node_t *n);

node_t *node_new_if_func(void);

node_t *node_new_lambda_func(void);

void node_print(node_t *n);
void node_print_recursive(node_t *n );
void node_print_pretty(node_t *n);

bool node_reachable_from(node_t *src, node_t *dst);

/* call cb on all live nodes */
typedef int (*find_cb_t)(node_t *n, void *p);
int node_find_live(find_cb_t cb, void *p);
int node_find_free(find_cb_t cb, void *p);
int node_find_roots(find_cb_t cb, void *p);
int node_find_unproc(find_cb_t cb, void *p);
int node_find_reachable(find_cb_t cb, void *p);
int node_find_boundary(find_cb_t cb, void *p);

/* free unused nodes, returns # nodes freed */
void node_gc(void);
//void node_sanity(void);
void node_gc_state(void);

#endif
