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
	case NODE_LIST:
#if defined(NODE_GC_TRACING)
		printf("links_cb: lst %p links to c=%p n=%p\n",
		       n, n->dat.list.child, n->dat.list.next);
#endif
		cb(n->dat.list.child, p);
		cb(n->dat.list.next, p);
		break;
	case NODE_LAMBDA:
#if defined(NODE_GC_TRACING)
		printf("links_cb: lam %p links to e=%p v=%p e=%p\n",
		       n, n->dat.lambda.env, n->dat.lambda.vars, n->dat.lambda.vars);
#endif
		cb(n->dat.lambda.env, p);
		cb(n->dat.lambda.vars, p);
		cb(n->dat.lambda.expr, p);
		break;
	case NODE_QUOTE:
#if defined(NODE_GC_TRACING)
		printf("links_bc: q %p links to v=%p\n", n, n->dat.quote.val);
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

node_t *node_retain(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	return n;
}

void node_release(node_t *n)
{
	(void) n;
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
}

void node_retrel(node_t *n)
{
	(void) n;
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
}

void node_forget(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n && memory_gc_isroot(&g_memstate, n)) {
		memory_gc_unlock(&g_memstate, n);
	}
}

void node_remember(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n) {
		memory_gc_lock(&g_memstate, n);
	}
}

bool node_is_remembered(node_t *n)
{
	return memory_gc_isroot(&g_memstate, n);
}

node_t *node_new_list(node_t *c, node_t *n)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.list.child = node_retain(c);
	ret->dat.list.next = node_retain(n);
	ret->type = NODE_LIST;
#if defined(NODE_INIT_TRACING)
	printf("init list %p (%p, %p)\n", ret, c, n);
#endif
	return ret;
}

void node_patch_list_next(node_t *n, node_t *newnext)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	node_release(n->dat.list.next);
	n->dat.list.next = node_retain(newnext);
}

void node_patch_list_child(node_t *n, node_t *newchld)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	node_release(n->dat.list.child);
	n->dat.list.child = node_retain(newchld);
}

node_t *node_child_noref(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n) {
		assert(n->type == NODE_LIST);
		return n->dat.list.child;
	}
	return NULL;
}

node_t *node_next_noref(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	if(n) {
		assert(n->type == NODE_LIST);
		return n->dat.list.next;
	}
	return NULL;
}

node_t *node_new_lambda(node_t *env, node_t *vars, node_t *expr)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.lambda.env = node_retain(env);
	ret->dat.lambda.vars = node_retain(vars);
	ret->dat.lambda.expr = node_retain(expr);
	ret->type = NODE_LAMBDA;
#if defined(NODE_INIT_TRACING)
	printf("init lambda %p (e=%p v=%p, x=%p)\n", ret, env, vars, expr);
#endif
	memory_gc_advise_link(&g_memstate, ret->dat.lambda.env);
	return ret;
}

node_t *node_lambda_env_noref(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.env;
}

node_t *node_lambda_vars_noref(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.vars;
}

node_t *node_lambda_expr_noref(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.expr;
}

node_t *node_new_value(uint64_t val)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.value = val;
	ret->type = NODE_VALUE;
#if defined(NODE_INIT_TRACING)
	printf("init value %p (%llu)\n", ret, (unsigned long long) val);
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

node_t *node_new_symbol(char *name)
{
	node_t *ret;
	assert(ret = node_new());
	strncpy(ret->dat.name, name, sizeof(ret->dat.name));
	ret->type = NODE_SYMBOL;
#if defined(NODE_INIT_TRACING)
	printf("init symbol %p (\"%s\")\n", ret, ret->dat.name);
#endif
	return ret;
}

char *node_name(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type == NODE_SYMBOL);
	return n->dat.name;
}

node_t *node_new_builtin(builtin_t func)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.func = func;
	ret->type = NODE_BUILTIN;
#if defined(NODE_INIT_TRACING)
	printf("init builtin %p (%p)\n", ret, ret->dat.func);
#endif
	return ret;
}

builtin_t node_func(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(n->type == NODE_BUILTIN);
	return n->dat.func;
}

node_t *node_new_quote(node_t *val)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.quote.val = node_retain(val);
	ret->type = NODE_QUOTE;
#if defined(NODE_INIT_TRACING)
	printf("init quote %p (%p)\n", ret, ret->dat.quote.val);
#endif
	return ret;
}

node_t *node_quote_val_noref(node_t *n)
{
#if defined(NODE_INCREMENTAL_GC)
	memory_gc_iterate(&g_memstate);
#endif
	assert(node_type(n) == NODE_QUOTE);
	return n->dat.quote.val;
}

node_t *node_new_if_func(void)
{
	node_t *ret;
	assert(ret = node_new());
	ret->type = NODE_IF_FUNC;
#if defined(NODE_INIT_TRACING)
	printf("init if_func\n");
#endif
	return ret;
}

node_t *node_new_lambda_func(void)
{
	node_t *ret;
	assert(ret = node_new());
	ret->type = NODE_LAMBDA_FUNC;
#if defined(NODE_INIT_TRACING)
	printf("init lambda_func\n");
#endif
	return ret;
}

void node_print(node_t *n)
{
	if(!n) {
		printf("node NULL\n");
	} else {
		printf("node %p : ", n);

		switch(n->type) {
		case NODE_LIST:
			printf("list child=%p next=%p",
			       n->dat.list.child, n->dat.list.next);
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
		case NODE_BUILTIN:
			printf("builtin %p", n->dat.func);
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
	if(n->type == NODE_LIST) {
		if(n->dat.list.child) node_print_recursive(n->dat.list.child);
		if(n->dat.list.next) node_print_recursive(n->dat.list.next);
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
		case NODE_LIST:
			printf("(");
			node_print_pretty(n->dat.list.child);
			printf(" . ");
			node_print_pretty(n->dat.list.next);
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
		case NODE_BUILTIN:
			printf("builtin:%p", n->dat.func);
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
