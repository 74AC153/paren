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

/* garbage collector globals */

static dlist_t live_list = DLIST_STATIC_INITIALIZER(live_list);
static size_t num_live = 0;

static dlist_t free_list = DLIST_STATIC_INITIALIZER(free_list);
static size_t num_free = 0;

static memcell_t *roots_cursor = NULL;
static dlist_t gc_roots_list = DLIST_STATIC_INITIALIZER(gc_roots_list);
static dlist_t gc_boundary_list = DLIST_STATIC_INITIALIZER(gc_boundary_list);
static dlist_t gc_white_list = DLIST_STATIC_INITIALIZER(gc_roots_list);
static dlist_t gc_black_list = DLIST_STATIC_INITIALIZER(gc_roots_list);
static dlist_t *gc_reachable_list = &gc_white_list;
static dlist_t *gc_unproc_list = &gc_black_list;

void node_gc_addroot(node_t *n)
{
	memcell_t *mc = node_to_memcell(n);
	if(dlnode_owner(&(mc->hdr)) != &gc_roots_list) {
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(&gc_roots_list, &(mc->hdr));
	}
}

void node_gc_advise_link(node_t *n)
{
	memcell_t *mc = node_to_memcell(n);
	if(dlnode_owner(&(mc->hdr)) == gc_unproc_list) {
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(gc_reachable_list, &(mc->hdr));
	}
}

void node_gc_remroot(node_t *n)
{
	memcell_t *mc = node_to_memcell(n);
	assert(dlnode_owner(&(mc->hdr)) == &gc_roots_list);
	dlnode_remove(&(mc->hdr));
	dlist_insertlast(gc_reachable_list, &(mc->hdr));
}

static void move_unproc_link_to_boundary(memcell_t *mc)
{
	if(dlnode_owner(&(mc->hdr)) == gc_unproc_list) {
		dlist_insertlast(&gc_boundary_list, dlnode_remove(&(mc->hdr)));
	}
}

static void move_unproc_links_to_boundary(node_t *n)
{
	switch(node_type(n)) {
	case NODE_LIST:
		move_unproc_link_to_boundary(node_to_memcell(n->dat.list.child));
		move_unproc_link_to_boundary(node_to_memcell(n->dat.list.next));
		break;
	case NODE_LAMBDA:
		move_unproc_link_to_boundary(node_to_memcell(n->dat.lambda.env));
		move_unproc_link_to_boundary(node_to_memcell(n->dat.lambda.vars));
		move_unproc_link_to_boundary(node_to_memcell(n->dat.lambda.expr));
		break;
	case NODE_QUOTE:
		move_unproc_link_to_boundary(node_to_memcell(n->dat.quote.val));
		break;
	default:
		break;
	}
}

void node_gc_iterate(void)
{
	/* first check to see if there are any boundary nodes to process */
	if(! dlist_is_empty(&gc_boundary_list)) {
		memcell_t *mc = (memcell_t *) dlist_first(&gc_boundary_list);
		move_unproc_links_to_boundary(&(mc->n));
		dlnode_remove(&(mc->hdr));
		dlist_insertrear(gc_reachable_list, &(mc->hdr));
		return;
	}

	/* if not, process a root node */
	if(! roots_cursor || dlnode_is_terminal(&(roots_cursor->hdr))) {
		roots_cursor = dlist_first(&gc_roots_list);
	} else {
		roots_cursor = (memcell_t *) dlnode_next(&(roots_cursor->hdr));
	}
	if(! dlnode_is_terminal(roots_cursor)) {
		move_unproc_links_to_boundary(&(roots_cursor->n));
		return;
	}

	/* if no more root nodes to process, begin processing the unreachables */
	if(! dlist_is_empty(gc_unproc_list)) {
		memcell_t *mc = (memcell_t *) dlist_first(gc_unproc_list);
		dlist_insertlast(&free_list, dlnode_remove(&(mc->hdr)));
		return;
	}

	/* when unreachables are processed, reset state */
	roots_cursor = dlist_first(&gc_roots_list);	
	if(gc_reachable_list == &gc_white_list) {
		gc_unproc_list = gc_reachable_list;
		gc_reachable_list = &gc_black_list;
	} else {
		gc_unproc_list = gc_reachable_list;
		gc_reachable_list = &gc_white_list;
	}
}

node_t *gc_request_node(void)
{
	memcell_t *mc;
	if(! dlist_is_empty(&free_list)) {
		mc = (memcell_t *) dlnode_remove(dlist_first(&free_list));
	} else {
		mc = (memcell_t *) dlnode_init(malloc(sizeof(memcell_t)));
#if defined(ALLOC_DEBUG)
		printf("malloc node %p\n", &(mc->n));
#endif
	}
	dlist_insertfirst(&gc_roots_list, &(mc->hdr));
	return &(mc->n);
}

/* garbage collector end */


#if 0
enum {
	GC_STATE_UNINITIALIZED,
	GC_STATE_TRAVERSE,
	GC_STATE_ROOTS,
	GC_STATE_CLEANUP
} gc_state = GC_STATE_UNINITIALIZED;

void node_gc_iterate(void)
{
	static memcell_t *roots_cursor, *boundary_cursor, *unproc_cursor;

	switch(gc_state) {
	case GC_STATE_UNINITIALIZED:
		roots_cursor = (memcell_t *) dlist_first(&gc_roots_list);
		gc_state = GC_STATE_ROOTS;
		break;
	case GC_STATE_ROOTS:
		if(dlnode_is_terminal(&(roots_cursor->hdr))) {
			boundary_cursor = (memcell_t *) dlist_first(&gc_boundary_list);
			gc_state = GC_STATE_TRAVERSE;
		} else {
			move_unproc_links_to_boundary(&(roots_cursor->n));
			roots_cursor = (memcell_t *) dlnode_next(&(roots_cursor->hdr));
		}
		break;
	case GC_STATE_TRAVERSE:
		if(dlnode_is_terminal(&(boundary_cursor->hdr))) {
			unproc_cursor = (memcell_t *) dlist_first(gc_unproc_list);
			gc_state = GC_STATE_CLEANUP;
		} else {
			memcell_t *mc = roots_cursor;
			move_unproc_links_to_boundary(&(boundary_cursor->n));
			boundary_cursor = (memcell_t *)dlnode_next(&(boundary_cursor->hdr));
			dlist_insertlast(gc_reachable_list, &(mc->hdr));
		}
		break;
	case GC_STATE_CLEANUP:
		if(dlnode_is_terminal(&(unproc_cursor->hdr))) {
			if(gc_reachable_list == &gc_white_list) {
				gc_unproc_list = gc_reachable_list;
				gc_reachable_list = &gc_black_list;
			} else {
				gc_unproc_list = gc_reachable_list;
				gc_reachable_list = &gc_white_list;
			}
			gc_state = GC_STATE_UNINITIALIZED;
		} else {
			memcell_t *mc = unproc_cursor;
			unproc_cursor = (memcell_t *) dlnode_next(&(unproc_cursor->hdr));
			dlist_insertlast(&free_list, dlnode_remove(&(mc->hdr)));
		}
		break;
	}
}
#endif


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
			case NODE_QUOTE:
				dec_rc_move_pend_z(&pend_list, n->dat.quote.val);
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

node_t *node_new_quote(node_t *val)
{
	node_t *ret;
	assert(ret = node_new());
	ret->dat.quote.val = node_retain(val);
	ret->type = NODE_QUOTE;
#if defined(ALLOC_DEBUG)
	printf("init quote %p (%p)\n", ret, ret->dat.quote.val);
#endif
	return ret;
}

node_t *node_quote_val_noref(node_t *n)
{
	assert(node_type(n) == NODE_QUOTE);
	return n->dat.quote.val;
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
		case NODE_QUOTE:
			printf("%p", n->dat.quote.val);
			break;
		case NODE_DEAD:
			printf("next=%p", n->dat.dead.next);
			break;
		default:
			break;
		}
		printf("\n");
		if(recursive) {
			if(n->type == NODE_LIST) {
				if(n->dat.list.child) node_print(n->dat.list.child, true);
				if(n->dat.list.next) node_print(n->dat.list.next, true);
			} else if(n->type == NODE_LAMBDA) {
		    	if(n->dat.lambda.env) node_print(n->dat.lambda.env, true);
				if(n->dat.lambda.vars) node_print(n->dat.lambda.vars, true);
				if(n->dat.lambda.expr) node_print(n->dat.lambda.expr, true);
			} else if(n->type == NODE_QUOTE) {
		    	if(n->dat.quote.val) node_print(n->dat.quote.val, true);
			}
		}
	}
}

node_t *node_new_if_func(void)
{
	node_t *ret;
	assert(ret = node_new());
	ret->type = NODE_IF_FUNC;
#if defined(ALLOC_DEBUG)
	printf("init if_func\n");
#endif
	return ret;
}

node_t *node_new_lambda_func(void)
{
	node_t *ret;
	assert(ret = node_new());
	ret->type = NODE_LAMBDA_FUNC;
#if defined(ALLOC_DEBUG)
	printf("init lambda_func\n");
#endif
	return ret;
}

bool node_reachable_from(node_t *src, node_t *dst)
{
	if(src == dst) {
		return true;
	}

	switch(node_type(src)) {
	case NODE_LIST:
		if(node_reachable_from(src->dat.list.child, dst)) {
			return true;
		}
		return node_reachable_from(src->dat.list.next, dst);
	case NODE_LAMBDA:
		if(node_reachable_from(src->dat.lambda.env, dst)) {
			return true;
		}
		if(node_reachable_from(src->dat.lambda.vars, dst)) {
			return true;
		}
		return node_reachable_from(src->dat.lambda.expr, dst);
	case NODE_QUOTE:
		return node_reachable_from(src->dat.quote.val, dst);
	default:
		return false;
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

