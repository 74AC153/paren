#include <stddef.h>
#include "libc_custom.h"
#include "freemem_cache.h"

fmcache_state_t *fmcache_state_init(
	void *p, 
	mem_allocator_fn_t mem_alloc,
	mem_free_fn_t mem_free,
	void *alloc_p)
{
	fmcache_state_t *state = (fmcache_state_t *) p;
	if(state) {
		dlist_init(&(state->free_list));
		state->mem_alloc = mem_alloc;
		state->mem_free = mem_free;
		state->alloc_p = alloc_p;
		state->alloc_count = 0;
	}

	return state;
}

static free_mem_cell_t *hdr_to_fmcell(dlnode_t *hdr)
{
	char *base = (char *) hdr - offsetof(free_mem_cell_t, u.hdr);
	return (free_mem_cell_t *) base;
}

static free_mem_cell_t *data_to_fmcell(void *data)
{
	char *base = (char *) data - offsetof(free_mem_cell_t, u.data);
	return (free_mem_cell_t *) base;
}

void fmcache_state_reset(fmcache_state_t * state)
{
	dlnode_t *n;
	while(! dlist_is_empty(&(state->free_list))) {
		n = dlnode_remove(dlist_first(&(state->free_list)));
		free(n);
	}
	state->alloc_count = 0;
}

void *fmcache_request(size_t len, void *fmcache_state)
{
	/* find a memory allocation that is large enough to hold len */
	dlnode_t *cursor;
	free_mem_cell_t *fmc;
	fmcache_state_t *state = (fmcache_state_t *)fmcache_state;

	for(cursor = dlist_first(&(state->free_list));
	    ! dlnode_is_terminal(cursor);
	    cursor = dlnode_next(cursor)) {
		fmc = hdr_to_fmcell(cursor);
		if(fmc->len >= len) {
#if defined(FMC_ALLOC_DEBUG)
		printf("fm_cache: reuse node 0x%p (data@0x%p) nfree=%llu\n",
		       fmc, &(fmc->u.data[0]), (unsigned long long) s->total_free);
#endif
			dlnode_remove(cursor);
			state->free_count--;
			return (void *) &(fmc->u.data[0]);
		}
	}
#if defined(FMC_ALLOC_DEBUG)
	printf("fm_cache: malloc node %p (%p) nalloc=%llu\n",
	       fmc, &(fmc->u.data[0]), (unsigned long long) s->total_alloc);
#endif
	fmc = malloc(len + offsetof(free_mem_cell_t, u));
	if(fmc) {
		fmc->len = len;
		state->alloc_count++;
		return (void *) &(fmc->u.data[0]);
	}
	return NULL;
}

void fmcache_return(void *data, void *fmcache_state)
{
	free_mem_cell_t *fmc = data_to_fmcell(data);
	fmcache_state_t *state = (fmcache_state_t *)fmcache_state;
	dlnode_init(&(fmc->u.hdr));
	dlist_insertfirst(&(state->free_list), &(fmc->u.hdr));
	state->free_count++;
#if defined(FMC_ALLOC_DEBUG)
	printf("fm_cache: return node %p (%p) nfree=%llu\n",
	       fmc, &(fmc->u.data[0]), (unsigned long long) s->total_free);
#endif
}

uintptr_t fmcache_count(fmcache_state_t *state)
{
	return state->alloc_count;
}
