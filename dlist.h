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

dlnode_t *dlnode_init(dlnode_t *n);

static inline bool dlnode_is_terminal(dlnode_t *n)
	{ return n == &(n->owner->hdr); }

static inline dlnode_t *dlnode_next(dlnode_t *n)
	{ return n->next; }

static inline dlnode_t *dlnode_prev(dlnode_t *n)
	{ return n->prev; }

static inline dlist_t *dlnode_owner(dlnode_t *n)
	{ return n->owner; }

void dlnode_insert(dlnode_t *prev, dlnode_t *n, dlnode_t *next);

void dlnode_move(dlnode_t *newprev, dlnode_t *n, dlnode_t *newnext);

static inline void dlnode_insertnext(dlnode_t *n, dlnode_t *next)
	{ dlnode_insert(n, next, n->next); }

static inline void dlnode_insertprev(dlnode_t *n, dlnode_t *prev)
	{ dlnode_insert(n->prev, prev, n); }

dlnode_t *dlnode_remove(dlnode_t *n);

void dlist_init(dlist_t *l);

static inline dlnode_t *dlist_first(dlist_t *l)
	{ return l->hdr.next; }

static inline dlnode_t *dlist_last(dlist_t *l)
	{ return l->hdr.prev; }

static inline void dlist_insertfirst(dlist_t *l, dlnode_t *n)
	{ dlnode_insert(&(l->hdr), n, l->hdr.next); }

static inline void dlist_insertlast(dlist_t *l, dlnode_t *n)
	{ dlnode_insert(l->hdr.prev, n, &(l->hdr)); }

static inline bool dlist_is_empty(dlist_t *l)
	{ return l->hdr.next == &(l->hdr); }

#define DLIST_FOR_FWD(LISTPTR, CURS) \
	for(CURS = dlist_first(LISTPTR); \
	    ! dlnode_is_terminal(CURS); \
	    CURS = dlnode_next(CURS))

#endif
