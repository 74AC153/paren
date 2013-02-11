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

dlist_t live_list = DLIST_STATIC_INITIALIZER(live_list);
dlist_t free_list = DLIST_STATIC_INITIALIZER(free_list);

char *node_type_names[] = {
	#define X(name) #name,
	NODE_TYPES
	#undef X
	NULL
};

static node_t *node_new(void)
{
	memcell_t *cell;
	node_t *ret;
	if(! dlist_is_empty(&free_list)) {
		cell = (memcell_t*) dlnode_remove(dlist_first(&free_list));
	} else {
		cell = (memcell_t*) dlnode_init(malloc(sizeof(struct node_memcell)));
#if defined(ALLOC_DEBUG)
		printf("malloc node %p\n", &(cell->n));
#endif
	}
	dlist_insertfirst(&live_list, &(cell->hdr));
	ret = &(cell->n);
	ret->refcount = 1;
	return ret;
}

static void node_delete(node_t *n)
{
	memcell_t *cell;
	assert(n->refcount == 0);
#if defined(ALLOC_DEBUG)
	printf("deleting ");
	node_print(n, false);
#endif
	cell = (((void *)n) - offsetof(memcell_t, n));
	dlnode_remove(&(cell->hdr));
	dlist_insertlast(&free_list, &(cell->hdr));
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

void node_release(node_t *n)
{
	if(n) {
		assert(n->refcount > 0);
		n->refcount--;
#if defined(REFCOUNT_DEBUG)
		printf("node %p ref-- <- %llu\n", n, (unsigned long long) n->refcount);
#endif
		if(! n->refcount) {
			if(n->type == NODE_LIST) {
				node_release(n->dat.list.next);
				node_release(n->dat.list.child);
			} 
			node_delete(n);
		}
	}
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

node_t *node_child(node_t *n)
{
	return node_retain(node_child_noref(n));
}

node_t *node_next(node_t *n)
{
	return node_retain(node_next_noref(n));
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
		case NODE_SYMBOL:
			printf("name: %s", n->dat.name);
			break;
		case NODE_VALUE:
			printf("value: %llu", (unsigned long long) n->dat.value);
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

void node_gc()
{
	memcell_t *cell;
	
	for(cell = (memcell_t *) dlist_first(&free_list);
	    ! dlnode_is_terminal(&(cell->hdr));
	    cell = (memcell_t *) dlist_first(&free_list)) {
		dlnode_remove(&(cell->hdr));
		free(cell);
	}
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
