#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "dlist.h"
#include "node.h"
#include "memory.h"

char *node_type_names[] = {
	#define X(name) #name,
	NODE_TYPES
	#undef X
	NULL
};

memory_state_t g_memstate;

static void links_cb(void (*cb)(void *link, void *p), void *data, void *p)
{
	node_t *n = (node_t *) data;

	switch(node_type(n)) {
	case NODE_CONS:
#if defined(NODE_GC_TRACING)
		printf("node links_cb: cons %p links to c=%p n=%p\n",
		       n, n->dat.cons.car, n->dat.cons.cdr);
#endif
		cb(n->dat.cons.car, p);
		cb(n->dat.cons.cdr, p);
		break;
	case NODE_LAMBDA:
#if defined(NODE_GC_TRACING)
		printf("node links_cb: lam %p links to e=%p v=%p e=%p\n",
		       n, n->dat.lambda.env, n->dat.lambda.vars, n->dat.lambda.vars);
#endif
		cb(n->dat.lambda.env, p);
		cb(n->dat.lambda.vars, p);
		cb(n->dat.lambda.expr, p);
		break;
	case NODE_QUOTE:
#if defined(NODE_GC_TRACING)
		printf("node links_cb: q %p links to v=%p\n", n, n->dat.quote.val);
#endif
		cb(n->dat.quote.val, p);
		break;
	default:
		break;
	}
}

static void node_print_wrap(void *p)
{
	node_print((node_t *) p);
}

void nodes_initialize()
{
	memory_state_init(&g_memstate, sizeof(node_t), links_cb, node_print_wrap);
}

static node_t *node_new(void)
{
	node_t *n;
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	n = memory_request(&g_memstate);
	return n;
}

nodetype_t node_type(node_t *n)
{
	if(n) return n->type;
	return NODE_NIL;
}

static node_t *node_retain(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n) {
		memory_gc_advise_new_link(&g_memstate, n);
	}
	return n;
}

static void node_release(node_t *n)
{
	(void) n;
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n) {
		memory_gc_advise_stale_link(&g_memstate, n);
	}
}

void node_droproot(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n && memory_gc_isroot(&g_memstate, n)) {
		memory_gc_unlock(&g_memstate, n);
	}
}

void node_makeroot(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n) {
		memory_gc_lock(&g_memstate, n);
	}
}

bool node_isroot(node_t *n)
{
	return memory_gc_isroot(&g_memstate, n);
}

node_t *node_cons_new(node_t *car, node_t *cdr)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.cons.car = node_retain(car);
	ret->dat.cons.cdr = node_retain(cdr);
	ret->type = NODE_CONS;
#if defined(NODE_INIT_TRACING)
	printf("node init cons %p (%p, %p)\n", ret, car, cdr);
#endif
	return ret;
}

void node_cons_patch_car(node_t *n, node_t *newcar)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n);
	assert(n->type == NODE_CONS);
	node_release(n->dat.cons.car);
	n->dat.cons.car = node_retain(newcar);
}

void node_cons_patch_cdr(node_t *n, node_t *newcdr)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n);
	assert(n->type == NODE_CONS);
	node_release(n->dat.cons.cdr);
	n->dat.cons.cdr = node_retain(newcdr);
}

node_t *node_cons_car(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n) {
		assert(n->type == NODE_CONS);
		return n->dat.cons.car;
	}
	return NULL;
}

node_t *node_cons_cdr(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n) {
		assert(n->type == NODE_CONS);
		return n->dat.cons.cdr;
	}
	return NULL;
}

node_t *node_lambda_new(node_t *env, node_t *vars, node_t *expr)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.lambda.env = node_retain(env);
	ret->dat.lambda.vars = node_retain(vars);
	ret->dat.lambda.expr = node_retain(expr);
	ret->type = NODE_LAMBDA;
#if defined(NODE_INIT_TRACING)
	printf("node init lambda %p (e=%p v=%p, x=%p)\n", ret, env, vars, expr);
#endif
	return ret;
}

node_t *node_lambda_env(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.env;
}

node_t *node_lambda_vars(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.vars;
}

node_t *node_lambda_expr(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.expr;
}

node_t *node_value_new(uint64_t val)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.value = val;
	ret->type = NODE_VALUE;
#if defined(NODE_INIT_TRACING)
	printf("node init value %p (%llu)\n", ret, (unsigned long long) val);
#endif
	return ret;
}

uint64_t node_value(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type == NODE_VALUE);
	return n->dat.value;
}

node_t *node_symbol_new(char *name)
{
	node_t *ret;
	assert(ret = node_new());
	strncpy(ret->dat.name, name, sizeof(ret->dat.name));
	ret->type = NODE_SYMBOL;
#if defined(NODE_INIT_TRACING)
	printf("node init symbol %p (\"%s\")\n", ret, ret->dat.name);
#endif
	return ret;
}

char *node_symbol_name(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type == NODE_SYMBOL);
	return n->dat.name;
}

node_t *node_foreign_new(foreign_t func)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.func = func;
	ret->type = NODE_FOREIGN;
#if defined(NODE_INIT_TRACING)
	printf("node init foreign %p (%p)\n", ret, ret->dat.func);
#endif
	return ret;
}

foreign_t node_foreign_func(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type == NODE_FOREIGN);
	return n->dat.func;
}

node_t *node_quote_new(node_t *val)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.quote.val = node_retain(val);
	ret->type = NODE_QUOTE;
#if defined(NODE_INIT_TRACING)
	printf("node init quote %p (%p)\n", ret, ret->dat.quote.val);
#endif
	return ret;
}

node_t *node_quote_val(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(node_type(n) == NODE_QUOTE);
	return n->dat.quote.val;
}

node_t *node_if_func_new(void)
{
	node_t *ret;
	assert(ret = node_new());
	ret->type = NODE_IF_FUNC;
#if defined(NODE_INIT_TRACING)
	printf("node init if_func\n");
#endif
	return ret;
}

node_t *node_lambda_func_new(void)
{
	node_t *ret;
	assert(ret = node_new());
	ret->type = NODE_LAMBDA_FUNC;
#if defined(NODE_INIT_TRACING)
	printf("node init lambda_func\n");
#endif
	return ret;
}

void node_print(node_t *n)
{
	if(!n) {
		printf("node NULL\n");
	} else {
		printf("node@%p: ", n);

		switch(n->type) {
		case NODE_CONS:
			printf("cons car=%p cdr=%p",
			       n->dat.cons.car, n->dat.cons.cdr);
			break;
		case NODE_LAMBDA:
			printf("lam env=%p vars=%p expr=%p",
			       n->dat.lambda.env, n->dat.lambda.vars, n->dat.lambda.expr);
			break;
		case NODE_SYMBOL:
			printf("sym %s", n->dat.name);
			break;
		case NODE_VALUE:
			printf("value %llu", (unsigned long long) n->dat.value);
			break;
		case NODE_FOREIGN:
			printf("foreign %p", n->dat.func);
			break;
		case NODE_QUOTE:
			printf("quote %p", n->dat.quote.val);
			break;
		default:
			break;
		}
	}
}

void node_print_recursive(node_t *n )
{
	node_print(n);
	printf("\n");
	if(n->type == NODE_CONS) {
		if(n->dat.cons.car) node_print_recursive(n->dat.cons.car);
		if(n->dat.cons.cdr) node_print_recursive(n->dat.cons.cdr);
	} else if(n->type == NODE_LAMBDA) {
    	if(n->dat.lambda.env) node_print_recursive(n->dat.lambda.env);
		if(n->dat.lambda.vars) node_print_recursive(n->dat.lambda.vars);
		if(n->dat.lambda.expr) node_print_recursive(n->dat.lambda.expr);
	} else if(n->type == NODE_QUOTE) {
    	if(n->dat.quote.val) node_print_recursive(n->dat.quote.val);
	}
}


void node_print_pretty(node_t *n)
{
		switch(node_type(n)) {
		case NODE_NIL:
			printf("()");
			break;
		case NODE_CONS:
			printf("(");
			node_print_pretty(n->dat.cons.car);
			printf(" . ");
			node_print_pretty(n->dat.cons.cdr);
			printf(")");
			break;
		case NODE_LAMBDA:
			printf("(lambda . (");
			node_print_pretty(n->dat.lambda.vars);
			printf(" . ");
			node_print_pretty(n->dat.lambda.expr);
			printf("))");
			break;
		case NODE_SYMBOL:
			printf("%s", n->dat.name);
			break;
		case NODE_VALUE:
			printf("%llu", (unsigned long long) n->dat.value);
			break;
		case NODE_FOREIGN:
			printf("foreign:%p", n->dat.func);
			break;
		case NODE_QUOTE:
			printf("'");
			node_print_pretty(n->dat.quote.val);
			break;
		case NODE_IF_FUNC:
			printf("if");
			break;
		case NODE_LAMBDA_FUNC:
			printf("lambda");
			break;
		}
}

void node_gc(void)
{
	while(! memory_gc_iterate(&g_memstate));
}

void node_gc_state(void)
{
	memory_gc_print_state(&g_memstate);
}
