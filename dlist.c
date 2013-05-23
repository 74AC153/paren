#include <assert.h>
#include "dlist.h"

dlnode_t *dlnode_init(dlnode_t *n)
{
	n->owner = NULL;
	n->prev = n->next = NULL;
	return n;
}

bool dlnode_is_terminal(dlnode_t *n)
{
	return n == &(n->owner->hdr);
}

dlnode_t *dlnode_next(dlnode_t *n)
{
	return n->next;
}

dlnode_t *dlnode_prev(dlnode_t *n)
{
	return n->prev;
}

dlist_t *dlnode_owner(dlnode_t *n)
{
	return n->owner;
}

void dlnode_insertnext(dlnode_t *n, dlnode_t *next)
{
	dlnode_t *n_next;
	assert(next->owner == NULL);
	assert(n->owner != NULL);
	n_next = n->next;
	next->next = n_next;
	n_next->prev = next;
	n->next = next;
	next->prev = n;
	next->owner = n->owner;
}

void dlnode_insertprev(dlnode_t *n, dlnode_t *prev)
{
	dlnode_t *n_prev;
	assert(prev->owner == NULL);
	assert(n->owner != NULL);
	n_prev = n->prev;
	prev->prev = n_prev;
	n_prev->next = prev;
	n->prev = prev;
	prev->next = n;
	prev->owner = n->owner;
}

dlnode_t *dlnode_remove(dlnode_t *n)
{
	dlnode_t *n_prev;
	dlnode_t *n_next;
	assert(n->owner != NULL);
	n_prev = n->prev;
	n_next = n->next;
	n_prev->next = n_next;
	n_next->prev = n_prev;
	n->next = n->prev = NULL;
	n->owner = NULL;
	return n;
}

void dlist_init(dlist_t *l)
{
	l->hdr.prev = l->hdr.next = &(l->hdr);
	l->hdr.owner = l;
}

dlnode_t *dlist_first(dlist_t *l)
{
	return l->hdr.next;
}

dlnode_t *dlist_last(dlist_t *l)
{
	return l->hdr.prev;
}

void dlist_insertfirst(dlist_t *l, dlnode_t *n)
{
	dlnode_insertnext(&(l->hdr), n);
}

void dlist_insertlast(dlist_t *l, dlnode_t *n)
{
	dlnode_insertprev(&(l->hdr), n);
}

bool dlist_is_empty(dlist_t *l)
{
	return l->hdr.next == &(l->hdr);
}
