#if ! defined(FREEMEM_CACHE_H)
#define MEMALLOC_H

#include "dlist.h"

#include "allocator_def.h"

typedef struct {
	size_t len;
	union {
		dlnode_t hdr;
		int data[0];
	} u;
} free_mem_cell_t;

typedef struct {
	dlist_t free_list;
	mem_allocator_fn_t mem_alloc;
	mem_free_fn_t mem_free;
	void *alloc_p;
	uintptr_t alloc_count;
	uintptr_t free_count;
} fmcache_state_t;

fmcache_state_t *fmcache_state_init(
	void *p, 
	mem_allocator_fn_t mem_alloc,
	mem_free_fn_t mem_free,
	void *alloc_p);

void fmcache_state_reset(fmcache_state_t *state);

void *fmcache_request(fmcache_state_t *state, size_t len);
void fmcache_return(fmcache_state_t *state, void *p);

uintptr_t fmcache_count(fmcache_state_t *state);

#endif
