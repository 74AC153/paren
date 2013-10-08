#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "dlist.h"
#include "stream.h"
#include "allocator_def.h"

typedef void (*data_fin_t)(void *data);

typedef struct
{
	dlnode_t hdr;
	struct memory_state *state;
	uintptr_t refcount;
	data_fin_t fin;
	struct {
		bool live:1;
		bool locked:1;
		bool searched:1;
	} mc_flags;
	int data[0];
} memcell_t;

/* must do something like
	foreach link from *data:
		cb(link, p)
*/
typedef void (*data_link_callback)(
	void (*cb)(void *link, void *p),
	void *data,
	void *p);

typedef void (*print_callback)(void *data, stream_t *stream);

typedef void (*init_callback)(void *data);

struct memory_state
{
	uintptr_t total_alloc;
#if ! defined(NO_GC_FREELIST)
	uintptr_t total_free;
#endif /* ! defined(NO_GC_FREELIST) */
	unsigned long long iter_count;
	unsigned long long cycle_count;
	unsigned int clean_cycles;
	unsigned long long skipped_clean_iters;
#if ! defined(NO_GC_FREELIST)
	dlist_t free_list;
#endif /* ! defined(NO_GC_FREELIST) */
	dlnode_t root_sentinel;
	dlist_t roots_list;
	dlist_t boundary_list;
	dlist_t free_pending_list;
	dlist_t white_list, black_list;
	dlist_t *reachable_listref;
	dlist_t *unproc_listref;
	init_callback i_cb;
	data_link_callback dl_cb;
	print_callback p_cb;
	struct {
		bool active:1;
	} ms_flags;
	mem_allocator_fn_t mem_alloc;
	mem_free_fn_t mem_free;
	void *mem_alloc_priv;
};
typedef struct memory_state memory_state_t;

/* initialize memory state */
void memory_state_init(
	memory_state_t *s,
	init_callback i_cb,
	data_link_callback dl_cb,
	print_callback p_cb,
	mem_allocator_fn_t mem_alloc,
	mem_free_fn_t mem_free,
	void *mem_alloc_priv);

void memory_state_reset(
	memory_state_t *s);

memory_state_t *data_to_memstate(void *data);

/* request memory from the GC */
void *memory_request(memory_state_t *s, size_t len);
/* attach finalizer callback to memory cell */
void memory_set_finalizer(void *data, data_fin_t fin);

bool memory_gc_is_locked(void *data);
/* this memory should never be freed (not NULL) */
void memory_gc_lock(memory_state_t *s, void *data);
/* this memory can be freed if not linked to (not NULL) */
void memory_gc_unlock(memory_state_t *s, void *data);

/* tell the GC that a new link to data has been created (NULL noop) */
void memory_gc_advise_new_link(memory_state_t *s, void *data);
/* tell the GC that a new link to data has been removed (NULL noop) */
void memory_gc_advise_stale_link(memory_state_t *s, void *data);

/* indicate whether data is a root node (false if NULL) */
bool memory_gc_isroot(memory_state_t *s, void *data);
/* indicate whether data is not in a freed node (false if NULL) */
bool memory_gc_islive(memory_state_t *s, void *data);

/* run the GC one iteration, return true on cycle complete */
bool memory_gc_iterate(memory_state_t *s);
/* run the GC one cycle */
static inline void memory_gc_cycle(memory_state_t *s)
	{ while(!memory_gc_iterate(s)); }

bool memcell_reachable(memory_state_t *s, memcell_t *dst);

void memory_gc_print_state(memory_state_t *state, stream_t *stream);

unsigned long long memory_gc_count_iters(memory_state_t *s);
unsigned long long memory_gc_count_cycles(memory_state_t *s);
uintptr_t memory_gc_count_total(memory_state_t *s);
uintptr_t memory_gc_count_free(memory_state_t *s);

#endif
