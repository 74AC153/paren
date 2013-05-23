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
bool dlnode_is_terminal(dlnode_t *n);
dlnode_t *dlnode_next(dlnode_t *n);
dlnode_t *dlnode_prev(dlnode_t *n);
dlist_t *dlnode_owner(dlnode_t *n);
void dlnode_insertnext(dlnode_t *n, dlnode_t *next);
void dlnode_insertprev(dlnode_t *n, dlnode_t *prev);
dlnode_t *dlnode_remove(dlnode_t *n);
void dlist_init(dlist_t *l);
dlnode_t *dlist_first(dlist_t *l);
dlnode_t *dlist_last(dlist_t *l);
void dlist_insertfirst(dlist_t *l, dlnode_t *n);
void dlist_insertlast(dlist_t *l, dlnode_t *n);
bool dlist_is_empty(dlist_t *l);

#define DLIST_FOR_FWD(LISTPTR, CURS) \
	for(CURS = dlist_first(LISTPTR); \
	    ! dlnode_is_terminal(CURS); \
	    CURS = dlnode_next(CURS))

#endif
