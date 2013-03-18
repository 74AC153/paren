#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <stdbool.h>
#include "token.h"

#define NODE_TYPES \
X(NODE_NIL) \
X(NODE_LIST) \
X(NODE_SYMBOL) \
X(NODE_VALUE) \
X(NODE_BUILTIN) \
X(NODE_DEAD)

typedef enum {
#define X(name) name,
NODE_TYPES
#undef X
} nodetype_t;

extern char *node_type_names[];

struct node;
typedef struct node node_t;

typedef int (*builtin_t)(struct node *args,
                         struct node **env,
                         struct node **result);

struct node {
	uint64_t refcount;
	nodetype_t type;
	union {
		struct { struct node *child, *next; } list;
		builtin_t func;
		char name[MAX_SYM_LEN];
		uint64_t value;
		struct { struct node *next; } dead;
	} dat;
};

nodetype_t node_type(node_t *n);
/* incr refcount */
node_t *node_retain(node_t *n);
/* decr refcount, then free if 0 */
void node_release(node_t *n);

/* returns node with refcount=1 */
node_t *node_new_list(node_t *c, node_t *n);
/* incr refcount of child and return child */
node_t *node_child(node_t *n);
/* incr refcount of next and return next*/
node_t *node_next(node_t *n);

/* thes don't bump refcounts */
node_t *node_child_noref(node_t *n);
node_t *node_next_noref(node_t *n);

/* returns node with refcount=1 */
node_t *node_new_value(uint64_t val);
uint64_t node_value(node_t *n);

/* returns node with refcount=1 */
node_t *node_new_symbol(char *name);
char *node_name(node_t *n);

/* returns node with refcount=1 */
node_t *node_new_builtin(builtin_t func);
builtin_t node_func(node_t *n);

void node_print(node_t *n, bool recursive);
void node_print_pretty(node_t *n);

/* call cb on all live nodes */
typedef int (live_cb_t)(node_t *n, void *p);
int node_find_live(live_cb_t cb, void *p);
int node_find_free(live_cb_t cb, void *p);

/* free unused nodes, returns # nodes freed */
size_t node_gc(void);

#endif
