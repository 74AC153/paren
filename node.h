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
	size_t refcount;
	nodetype_t type;
	uint32_t flags;
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

nodetype_t node_type(node_t *n);
/* incr refcount */
node_t *node_retain(node_t *n);
/* decr refcount, then free if 0 */
void node_release(node_t *n);
/* for when node was created but refcount never incremented */
void node_retrel(node_t *n);
uint64_t node_refcount(node_t *n);

/* returns node with refcount=1 */
node_t *node_new_list(node_t *c, node_t *n);
node_t *node_child_noref(node_t *n);
node_t *node_next_noref(node_t *n);
/* release node->next and retain newnext */
void node_patch_list_next(node_t *n, node_t *newnext);
void node_patch_list_child(node_t *n, node_t *newchld);

/* incr refcount of child and return child */
//node_t *node_child(node_t *n);
/* incr refcount of next and return next*/
//node_t *node_next(node_t *n);



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

void node_print(node_t *n, bool recursive);
void node_print_pretty(node_t *n);

/* call cb on all live nodes */
typedef int (live_cb_t)(node_t *n, void *p);
int node_find_live(live_cb_t cb, void *p);
int node_find_free(live_cb_t cb, void *p);

/* free unused nodes, returns # nodes freed */
size_t node_gc(void);
void node_sanity(void);

#endif
