#ifndef DLIST_H
#define DLIST_H

#include <stdbool.h>
#include <stdlib.h>

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
	DLIST_STATIC_INITIALIZER(list)
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

	DLIST_FOR_FWD(LISTPTR, CURS)
	typedef int (*iterate_cb_t)(dlnode_t *n, void *p);
	int dlist_iterate(dlist_t *l, iterate_cb_t cb, void *p);
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
	dlnode_t *n_next = n->next;
	next->next = n_next;
	n_next->prev = next;
	n->next = next;
	next->prev = n;
	next->owner = n->owner;
}

static inline
void dlnode_insertprev(dlnode_t *n, dlnode_t *prev)
{
	dlnode_t *n_prev = n->prev;
	prev->prev = n_prev;
	n_prev->next = prev;
	n->prev = prev;
	prev->next = n;
	prev->owner = n->owner;
}

static inline
dlnode_t *dlnode_remove(dlnode_t *n)
{
	dlnode_t *n_prev = n->prev;
	dlnode_t *n_next = n->next;
	n_prev->next = n_next;
	n_next->prev = n_prev;
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

#define DLIST_FOR_FWD(LISTPTR, CURS) \
	for(CURS = dlist_first(LISTPTR); \
	    ! dlnode_is_terminal(CURS); \
	    CURS = dlnode_next(CURS))

typedef int (*iterate_cb_t)(dlnode_t *n, void *p);
static inline
int dlist_iterate(dlist_t *l, iterate_cb_t cb, void *p)
{
	int status = 0;
	dlnode_t *n = dlist_first(l);
	
	for( ; ! dlnode_is_terminal(n) && !status; n = dlnode_next(n)) {
		status = cb(n, p);
	}
	
	return status;
}

#endif
