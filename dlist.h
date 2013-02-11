#ifndef DLIST_H
#define DLIST_H

#include <stdbool.h>

struct dlnode {
	struct dlnode *prev, *next;
	struct dlist *owner;
};
typedef struct dlnode dlnode_t;

struct dlist {
	dlnode_t hdr;
};
typedef struct dlist dlist_t;

#define DLIST_STATIC_INITIALIZER(LIST) \
	{ { &((LIST).hdr), &((LIST).hdr), &(LIST)} }

/*
	void dlist_init(void *p)
	dlnode_t *dlist_first(dlist_t *l)
	dlnode_t *dlist_last(dlist_t *l)
	void dlist_insertfirst(dlist_t *l, dlnode_t *n)
	void dlist_insertlast(dlist_t *l, dlnode_t *n)
	bool dlist_is_empty(dlist_t *l)

	dnode_t *dlnode_init(void *p)
	bool dlnode_is_terminal(dlnode_t *n)
	dlnode_t *dlnode_next(dlnode_t *n)
	dlnode_t *dlnode_prev(dlnode_t *n)
	dlist_t *dlnode_owner(dlnode_t *n)
	void dlnode_insertnext(dlnode_t *n, dlnode_t *next)
	void dlnode_insertprev(dlnode_t *n, dlnode_t *prev)
	dlnode_t *dlnode_remove(dlnode_t *n)
*/

static inline
dlnode_t *dlnode_init(dlnode_t *n)
{
	n->owner = NULL;
	n->prev = n->next = NULL;
	return n;
}

static inline
bool dlnode_is_terminal(dlnode_t *n)
{
	return n == &(n->owner->hdr);
}

static inline
dlnode_t *dlnode_next(dlnode_t *n)
{
	return n->next;
}

static inline
dlnode_t *dlnode_prev(dlnode_t *n)
{
	return n->prev;
}

static inline
dlist_t *dlnode_owner(dlnode_t *n)
{
	return n->owner;
}

static inline
void dlnode_insertnext(dlnode_t *n, dlnode_t *next)
{
	next->next = n->next;
	n->next->prev = next;
	n->next = next;
	next->prev = n;
	next->owner = n->owner;
}

static inline
void dlnode_insertprev(dlnode_t *n, dlnode_t *prev)
{
	prev->prev = n->prev;
	n->prev->next = prev;
	n->prev = prev;
	prev->next = n;
	prev->owner = n->owner;
}

static inline
dlnode_t *dlnode_remove(dlnode_t *n)
{
	n->prev->next = n->next;
	n->next->prev = n->prev;
	n->next = n->prev = NULL;
	n->owner = NULL;
	return n;
}


static inline
void dlist_init(dlist_t *l)
{
	l->hdr.prev = l->hdr.next = &(l->hdr);
	l->hdr.owner = l;
}

static inline
dlnode_t *dlist_first(dlist_t *l)
{
	return l->hdr.next;
}

static inline
dlnode_t *dlist_last(dlist_t *l)
{
	return l->hdr.prev;
}

static inline
void dlist_insertfirst(dlist_t *l, dlnode_t *n)
{
	dlnode_insertnext(&(l->hdr), n);
}

static inline
void dlist_insertlast(dlist_t *l, dlnode_t *n)
{
	dlnode_insertprev(&(l->hdr), n);
}

static inline
bool dlist_is_empty(dlist_t *l)
{
	return l->hdr.next == &(l->hdr);
}

#endif
