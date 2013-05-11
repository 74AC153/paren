#ifndef MEMORY_H
#define MEMORY_H

#include "dlist.h"

typedef struct
{
	dlnode_t hdr;
	bool locked;
	int data[0];
} memcell_t;

/* must do something like
	foreach link from *data:
		cb(link, p)
*/
typedef void (*data_link_callback)(void (*cb)(void *link, void *p), void *data, void *p);

typedef void (*print_callback)(void *data);

typedef struct
{
	size_t datasize;
	uintptr_t total_alloc, total_free;
	unsigned long long iter_count;
	dlist_t free_list;
	dlnode_t root_sentinel;
	dlist_t roots_list;
	dlist_t boundary_list;
	dlist_t white_list, black_list;
	dlist_t *reachable_listref;
	dlist_t *unproc_listref;
	data_link_callback dl_cb;
	print_callback p_cb;
} memory_state_t;

/* initialize memory state */
void memory_state_init(
	memory_state_t *s,
	size_t datasize,
	data_link_callback dl_cb,
	print_callback p_cb);

/* request memory from the GC */
void *memory_request(memory_state_t *s);

/* this memory is not linked to, is needed, and should never be linked to */
void memory_gc_lock(memory_state_t *s, void *data);
/* this memory was a root, but no longer has that status */
void memory_gc_unlock(memory_state_t *s, void *data);

/* tell the GC that this memory is linked to, if a root, remove root status */
void *memory_gc_advise_link(memory_state_t *s, void *data);

/* indicate whether data is a root node */
bool memory_gc_isroot(memory_state_t *s, void *data);
/* indicate whether data is not in a freed node */
bool memory_gc_islive(memory_state_t *s, void *data);

/* run the GC one iteration */
bool memory_gc_iterate(memory_state_t *s);
void memory_gc_print_state(memory_state_t *s);

#endif
