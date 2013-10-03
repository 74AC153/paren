#include <assert.h>
#include "dlist.h"

dlnode_t *dlnode_init(dlnode_t *n)
{
	n->owner = NULL;
	n->prev = n->next = NULL;
	return n;
}

static void dlnode_join(dlnode_t *n, dlnode_t *m)
{
	m->next = n;
	n->prev = m;
}

void dlnode_insert(dlnode_t *prev, dlnode_t *n, dlnode_t *next)
{
	assert(prev->next == next);
	assert(next->prev == prev);
	assert(prev->owner);
	assert(prev->owner == next->owner);
	assert(! n->owner);

	dlnode_join(prev, n);
	dlnode_join(n, next);

	n->owner = prev->owner;
}

void dlnode_move(dlnode_t *newprev, dlnode_t *n, dlnode_t *newnext)
{
	assert(newprev->next == newnext);
	assert(newnext->prev == newprev);
	assert(newprev->owner);
	assert(newprev->owner == newnext->owner);
	assert(n->owner);

	dlnode_join(n->prev, n->next);
	dlnode_join(newprev, n);
	dlnode_join(n, newnext);

	n->owner = newprev->owner;
}

dlnode_t *dlnode_remove(dlnode_t *n)
{
	dlnode_t *n_prev;
	dlnode_t *n_next;
	assert(n->owner != NULL);

	dlnode_join(n->prev, n->next);

	n->owner = NULL;
	return n;
}

void dlist_init(dlist_t *l)
{
	l->hdr.prev = l->hdr.next = &(l->hdr);
	l->hdr.owner = l;
}
