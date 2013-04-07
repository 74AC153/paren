#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "dlist.h"
#include "node.h"

struct node_memcell {
	dlnode_t hdr;
	node_t n;
};
typedef struct node_memcell memcell_t;

static inline
memcell_t *node_to_memcell(node_t *n)
{
	return (memcell_t *) (((void *) n) - offsetof(memcell_t, n));
}

dlist_t live_list = DLIST_STATIC_INITIALIZER(live_list);
size_t num_live = 0;
dlist_t free_list = DLIST_STATIC_INITIALIZER(free_list);
size_t num_free = 0;

#define NODE_FLAG_DEL_PENDING 0x1

char *node_type_names[] = {
	#define X(name) #name,
	NODE_TYPES
	#undef X
	NULL
};

static node_t *node_new(void)
{
	memcell_t *mc;
	node_t *ret;
	if(! dlist_is_empty(&free_list)) {
		mc = (memcell_t *) dlnode_remove(dlist_first(&free_list));
	} else {
		mc = (memcell_t *) dlnode_init(malloc(sizeof(memcell_t)));
#if defined(ALLOC_DEBUG)
		printf("malloc node %p\n", &(mc->n));
#endif
	}
	dlist_insertfirst(&live_list, &(mc->hdr));
	num_live++;
	ret = &(mc->n);
	ret->refcount = 0;
	ret->flags = 0;
	return ret;
}

uint64_t node_refcount(node_t *n)
{
	return n->refcount;
}

nodetype_t node_type(node_t *n)
{
	if(n) return n->type;
	return NODE_NIL;
}

node_t *node_retain(node_t *n)
{
	if(n) {
		n->refcount++;
#if defined(REFCOUNT_DEBUG)
		printf("node %p ref++ <- %llu\n", n, (unsigned long long) n->refcount);
#endif
		assert(n->refcount > 0); /* protect against wrap */
	}
	return n;
}

static void canary(void)
{

}

static void dec_rc_move_pend_z(dlist_t *pend_list, node_t *n)
{
	if(!n) return;

	if(! n->refcount) {
		assert(n->flags & NODE_FLAG_DEL_PENDING);
		return;
	}

	n->refcount --;
#if defined(REFCOUNT_DEBUG)
	printf("node %p ref-- <- %llu\n", n, (unsigned long long) n->refcount);
#endif

	if(! n->refcount) {
		canary();
		memcell_t *mc = node_to_memcell(n);
		n->flags |= NODE_FLAG_DEL_PENDING;
		dlist_insertlast(pend_list, dlnode_remove(&mc->hdr));
	}
}

void node_release(node_t *n)
{
	memcell_t *mc;
	dlist_t del_list = DLIST_STATIC_INITIALIZER(del_list);
	dlist_t pend_list = DLIST_STATIC_INITIALIZER(pend_list);

	dec_rc_move_pend_z(&pend_list, n);

	/* traverse all connections, mark for delete if applicable */
	while(! dlist_is_empty(&pend_list)) {
		mc = (memcell_t *) dlnode_remove(dlist_first(&pend_list));
		n = &mc->n;
		switch(n->type) {
			case NODE_LIST:
				dec_rc_move_pend_z(&pend_list, n->dat.list.next);
				dec_rc_move_pend_z(&pend_list, n->dat.list.child);
				break;
			case NODE_LAMBDA:
				dec_rc_move_pend_z(&pend_list, n->dat.lambda.vars);
				dec_rc_move_pend_z(&pend_list, n->dat.lambda.expr);
				dec_rc_move_pend_z(&pend_list, n->dat.lambda.env);
				break;
			default:
				assert(n->type != NODE_DEAD);
				break;
		}
		dlist_insertfirst(&del_list, &mc->hdr);
	}

	/* reset state and add to free list */
	while(! dlist_is_empty(&del_list)) {
		mc = (memcell_t *) dlnode_remove(dlist_first(&del_list));
		mc->n.flags &= ~NODE_FLAG_DEL_PENDING;
#if defined(ALLOC_DEBUG)
		printf("deleting ");
		node_print(&(mc->n), false);
#endif
		dlist_insertfirst(&free_list, &mc->hdr);
		num_live--;
		num_free++;
	}
}

void node_retrel(node_t *n)
{
	node_release(node_retain(n));
}

node_t *node_new_list(node_t *c, node_t *n)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.list.child = node_retain(c);
	ret->dat.list.next = node_retain(n);
	ret->type = NODE_LIST;
#if defined(ALLOC_DEBUG)
	printf("init list %p (%p, %p)\n", ret, c, n);
#endif
	return ret;
}

void node_patch_list_next(node_t *n, node_t *newnext)
{
	node_release(n->dat.list.next);
	n->dat.list.next = node_retain(newnext);
}

void node_patch_list_child(node_t *n, node_t *newchld)
{
	node_release(n->dat.list.child);
	n->dat.list.child = node_retain(newchld);
}

node_t *node_child_noref(node_t *n)
{
	if(n) {
		assert(n->type == NODE_LIST);
		return n->dat.list.child;
	}
	return NULL;
}

node_t *node_next_noref(node_t *n)
{
	if(n) {
		assert(n->type == NODE_LIST);
		return n->dat.list.next;
	}
	return NULL;
}

/*
node_t *node_child(node_t *n)
{
	return node_retain(node_child_noref(n));
}
*/

/*node_t *node_next(node_t *n)
{
	return node_retain(node_next_noref(n));
}
*/

node_t *node_new_lambda(node_t *env, node_t *vars, node_t *expr)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.lambda.env = node_retain(env);
	ret->dat.lambda.vars = node_retain(vars);
	ret->dat.lambda.expr = node_retain(expr);
	ret->type = NODE_LAMBDA;
#if defined(ALLOC_DEBUG)
	printf("init lambda %p (e=%p v=%p, x=%p)\n", ret, env, vars, expr);
#endif
	return ret;
}

node_t *node_lambda_env_noref(node_t *n)
{
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.env;
}

node_t *node_lambda_vars_noref(node_t *n)
{
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.vars;
}

node_t *node_lambda_expr_noref(node_t *n)
{
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.expr;
}

node_t *node_new_value(uint64_t val)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.value = val;
	ret->type = NODE_VALUE;
#if defined(ALLOC_DEBUG)
	printf("init value %p (%llu)\n", ret, (unsigned long long) val);
#endif
	return ret;
}

uint64_t node_value(node_t *n)
{
	assert(n->type == NODE_VALUE);
	return n->dat.value;
}

node_t *node_new_symbol(char *name)
{
	node_t *ret;
	assert(ret = node_new());
	strncpy(ret->dat.name, name, sizeof(ret->dat.name));
	ret->type = NODE_SYMBOL;
#if defined(ALLOC_DEBUG)
	printf("init symbol %p (\"%s\")\n", ret, ret->dat.name);
#endif
	return ret;
}

char *node_name(node_t *n)
{
	assert(n->type == NODE_SYMBOL);
	return n->dat.name;
}

node_t *node_new_builtin(builtin_t func)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.func = func;
	ret->type = NODE_BUILTIN;
#if defined(ALLOC_DEBUG)
	printf("init builtin %p (%p)\n", ret, ret->dat.func);
#endif
	return ret;
}

builtin_t node_func(node_t *n)
{
	assert(n->type == NODE_BUILTIN);
	return n->dat.func;
}

void node_print(node_t *n, bool recursive)
{
	if(!n) {
		printf("node NULL\n");
	} else {
		printf("node %p: ref=%llu type=%s ",
		       n, (unsigned long long) n->refcount, node_type_names[n->type]);

		switch(n->type) {
		case NODE_LIST:
			printf("child=%p next=%p", n->dat.list.child, n->dat.list.next);
			break;
		case NODE_LAMBDA:
			printf("env=%p vars=%p expr=%p",
			       n->dat.lambda.env,
			       n->dat.lambda.vars,
			       n->dat.lambda.expr);
			break;
		case NODE_SYMBOL:
			printf("%s", n->dat.name);
			break;
		case NODE_VALUE:
			printf("%llu", (unsigned long long) n->dat.value);
			break;
		case NODE_BUILTIN:
			printf("%p", n->dat.func);
			break;
		case NODE_DEAD:
			printf("next=%p", n->dat.dead.next);
			break;
		default:
			break;
		}
		printf("\n");
		if(recursive && n->type == NODE_LIST) {
			if(n->dat.list.child) node_print(n->dat.list.child, true);
			if(n->dat.list.next) node_print(n->dat.list.next, true);
		}
	}
}

void node_print_pretty(node_t *n)
{
		switch(node_type(n)) {
		case NODE_DEAD:
			printf("--dead--");
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
		}
}

size_t node_gc(void)
{
	memcell_t *cell;
	size_t count = 0;
	
	for(cell = (memcell_t *) dlist_first(&free_list);
	    ! dlnode_is_terminal(&(cell->hdr));
	    cell = (memcell_t *) dlist_first(&free_list)) {
		dlnode_remove(&(cell->hdr));
		free(cell);
		count++;
	}
	return count;
}

int node_find_live(live_cb_t cb, void *p)
{
	int status = 0;
	memcell_t *cell;
	
	for(cell = (memcell_t *) dlist_first(&live_list);
	    ! dlnode_is_terminal(&(cell->hdr)) && !status;
	    cell = (memcell_t *) dlnode_next(&(cell->hdr))) {
		status = cb(&(cell->n), p);
	}
	
	return status;
}

int node_find_free(live_cb_t cb, void *p)
{
	int status = 0;
	memcell_t *cell;
	
	for(cell = (memcell_t *) dlist_first(&free_list);
	    ! dlnode_is_terminal(&(cell->hdr)) && !status;
	    cell = (memcell_t *) dlnode_next(&(cell->hdr))) {
		status = cb(&(cell->n), p);
	}
	
	return status;
}

void node_sanity(void)
{
	memcell_t *mc, *mc_test;
	node_t *n, *n_test;
	size_t links;

	printf("*** node sanity check ***\n");

	/* count links */
	for(mc = (memcell_t *) dlist_first(&live_list);
	    !dlnode_is_terminal(&(mc->hdr));
	    mc = (memcell_t *) dlnode_next(&(mc->hdr))) {
		n = &(mc->n);
		links = 0;
		for(mc_test = (memcell_t *) dlist_first(&live_list);
		    !dlnode_is_terminal(&(mc_test->hdr));
		    mc_test = (memcell_t *) dlnode_next(&(mc_test->hdr))) {
			n_test = &(mc_test->n);
			if(mc_test == mc) {
				continue;
			}
			switch(mc_test->n.type) {
			case NODE_LIST:
				if(n_test->dat.list.next == n) links++;
				if(n_test->dat.list.child == n) links++;
				break;
			case NODE_LAMBDA:
				if(n_test->dat.lambda.env == n) links++;
				if(n_test->dat.lambda.vars == n) links++;
				if(n_test->dat.lambda.expr == n) links++;
				break;
			default:
				break;
			}
		}
		if(links != n->refcount) {
			printf("warning: node %p has %u links but refcount %u\n",
			       n, (unsigned) links, (unsigned) n->refcount);
		}
	}

	/* ensure all links are live */
	for(mc = (memcell_t *) dlist_first(&live_list);
	    !dlnode_is_terminal(&(mc->hdr));
	    mc = (memcell_t *) dlnode_next(&(mc->hdr))) {
		n = &(mc->n);
		for(mc_test = (memcell_t *) dlist_first(&free_list);
		    !dlnode_is_terminal(&(mc_test->hdr));
		    mc_test = (memcell_t *) dlnode_next(&(mc_test->hdr))) {
			n_test = &(mc_test->n);
			switch(n->type) {
			case NODE_LIST:
				if(n->dat.list.next == n_test)
					printf("error: node %p has next ptr to free node %p\n",
					       &(mc->n), n_test);
				if(n->dat.list.child == n_test)
					printf("error: node %p has child ptr to free node %p\n",
					       &(mc->n), n_test);
				break;
			case NODE_LAMBDA:
				if(n->dat.lambda.env == n_test)
					printf("error: node %p has env ptr to free node %p\n",
					       &(mc->n), n_test);
				if(n->dat.lambda.vars == n_test)
					printf("error: node %p has vars ptr to free node %p\n",
					       &(mc->n), n_test);
				if(n->dat.lambda.expr == n_test)
					printf("error: node %p has expr ptr to free node %p\n",
					       &(mc->n), n_test);
				break;
			default:
				break;
			}

		}
	}
}

