#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#include "memory.h"

static inline
memcell_t *data_to_memcell(void *data)
{
	return (memcell_t *) (data - offsetof(memcell_t, data));
}

void memory_state_init(
	memory_state_t *s,
	size_t datasize,
	data_link_callback dl_cb,
	print_callback p_cb)
{
	s->datasize = datasize;
	dlist_init(&(s->free_list));
	dlist_init(&(s->roots_list));
	dlist_init(&(s->boundary_list));
	dlist_init(&(s->white_list));
	dlist_init(&(s->black_list));
	dlnode_init(&(s->root_sentinel));
	dlist_insertlast(&(s->roots_list), &(s->root_sentinel));
	s->reachable_listref = &(s->white_list);
	s->unproc_listref = &(s->black_list);
	s->dl_cb = dl_cb;
	s->p_cb = p_cb;
	s->total_alloc = 0;
	s->total_free = 0;
	s->iter_count = 0;
}

void *memory_request(memory_state_t *s)
{
	memcell_t *mc;
	if(! dlist_is_empty(&(s->free_list))) {
		mc = (memcell_t *) dlnode_remove(dlist_first(&(s->free_list)));
		assert(! mc->locked);
		s->total_free--;
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): reuse node %p\n", s->iter_count, mc);
#endif
	} else {
		mc = (memcell_t *) dlnode_init(malloc(sizeof(memcell_t)+s->datasize));
		mc->locked = false;
		s->total_alloc++;
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): malloc node %p\n", s->iter_count, mc);
#endif
	}
	dlist_insertlast(&(s->roots_list), &(mc->hdr));
	return mc->data;
}

void memory_gc_lock(memory_state_t *s, void *data)
{
	memcell_t *mc = data_to_memcell(data);
	mc->locked = true;
	if(dlnode_owner(&(mc->hdr)) != &(s->roots_list)) {
#if defined(GC_TRACING)
		printf("gc: add root %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
		dlnode_remove(&(mc->hdr));
		dlnode_insertprev(&(s->root_sentinel), &(mc->hdr));
	} else {
#if defined(GC_TRACING)
		printf("gc: add root %p (noop) ", mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
	}
}

void memory_gc_unlock(memory_state_t *s, void *data)
{
	memcell_t *mc = data_to_memcell(data);
	assert(dlnode_owner(&(mc->hdr)) == &(s->roots_list));
#if defined(GC_TRACING)
		printf("gc: rem root %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
	dlnode_remove(&(mc->hdr));
	dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	mc->locked = false;
}

bool memory_gc_isroot(memory_state_t *s, void *data)
{
	bool status;
	memcell_t *mc = data_to_memcell(data);
	status = dlnode_owner(&(mc->hdr)) == &(s->roots_list);
	return status;
}

bool memory_gc_islive(memory_state_t *s, void *data)
{
	memcell_t *mc = data_to_memcell(data);
	return dlnode_owner(&(mc->hdr)) != &(s->free_list);
}

void *memory_gc_advise_link(memory_state_t *s, void *data)
{
	memcell_t *mc;
	if(!data) {
#if defined(GC_TRACING)
		printf("gc: advise link null (noop)\n");
#endif
		return data;
	}
	mc = data_to_memcell(data);

	/* linked data should never be locked or in the free list */
	assert(dlnode_owner(&(mc->hdr)) != &(s->free_list));


	if(dlnode_owner(&(mc->hdr)) == s->unproc_listref) {
#if defined(GC_TRACING)
		printf("gc: advise link unproc %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	} else if(dlnode_owner(&(mc->hdr)) == &(s->roots_list)) {
#if defined(GC_TRACING)
		printf("gc: advise link root %p%s", mc, mc->locked ? " (locked)" : " ");
		s->p_cb(mc->data);
		printf("\n");
#endif
		if(!mc->locked) {
			dlnode_remove(&(mc->hdr));
			dlist_insertlast(&(s->boundary_list), &(mc->hdr));
		}
	} else {
#if defined(GC_TRACING)
		printf("gc: advise link noop %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
	}
	return data;
}

static void dl_cb_gc_advise_link_wrap(void *link, void *p)
{
	memory_state_t *s = (memory_state_t *) p;
	memory_gc_advise_link(s, link);
}

/* returns true when a complete gc cycle has been completed */
bool memory_gc_iterate(memory_state_t *s)
{
	bool status = false;
	s->iter_count++;

	/* process boundary nodes */
	if(! dlist_is_empty(&(s->boundary_list))) {
		memcell_t *mc = (memcell_t *) dlist_first(&(s->boundary_list));
		assert(!mc->locked);
#if defined(GC_TRACING)
		printf("gc (%llu) iter reachable: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
		s->dl_cb(dl_cb_gc_advise_link_wrap, &(mc->data), s);
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(s->reachable_listref, &(mc->hdr));
		goto finish;
	}

	/* check to see if there are any root nodes to process */
	{
		dlnode_t *root_first = dlist_first(&(s->roots_list));
		if(root_first != &(s->root_sentinel)) {
			memcell_t *mc = (memcell_t *) root_first;
#if defined(GC_TRACING)
			printf("gc (%llu) iter root: %p ", s->iter_count, mc);
			s->p_cb(mc->data);
			printf("\n");
#endif
			s->dl_cb(dl_cb_gc_advise_link_wrap, &(mc->data), s);
			/* after processing, roate to end of list */
			dlist_insertlast(&(s->roots_list), dlnode_remove(&(mc->hdr)));
			goto finish;
		}
		dlist_insertlast(&(s->roots_list), dlnode_remove(root_first));
	}

	/* process unreachables */
	if(! dlist_is_empty(s->unproc_listref)) {
		memcell_t *mc = (memcell_t *) dlist_first(s->unproc_listref);
		assert(!mc->locked);
#if defined(GC_TRACING)
		printf("gc (%llu) iter unreachable: %p\n", s->iter_count, mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): free node %p\n", s->iter_count, mc);
#endif
		dlist_insertlast(&(s->free_list), dlnode_remove(&(mc->hdr)));
		s->total_free++;
		goto finish;
	}

	/* reset state */
#if defined(GC_TRACING)
	printf("gc iter ** done **: reset state\n");
#endif
	if(s->reachable_listref == &(s->white_list)) {
		s->unproc_listref = s->reachable_listref;
		s->reachable_listref = &(s->black_list);
	} else {
		s->unproc_listref = s->reachable_listref;
		s->reachable_listref = &(s->white_list);
	}
	status = true;

finish:
#if defined(GC_VERBOSE)
	memory_gc_print_state(s);
#endif
	return status;
}

void memory_gc_print_state(memory_state_t *s)
{
	dlnode_t *cursor;

	printf("memory state %p (%lu alloc %lu free %lu bytes total):\n",
	       s, (unsigned long) s->total_alloc, (unsigned long) s->total_free,
	       (unsigned long) ((sizeof(memcell_t)+s->datasize)*s->total_alloc));

	for(cursor = dlist_first(&(s->roots_list));
	    ! dlnode_is_terminal(cursor);
	    cursor = dlnode_next(cursor)) {
		if(cursor == &(s->root_sentinel)) {
			printf("   root (sentinel)\n");
		} else {
			memcell_t *mc = (memcell_t *) cursor;
			printf("   root %p ", mc);
			s->p_cb(mc->data);
			printf("\n");
		}
	}

	for(cursor = dlist_first(&(s->boundary_list));
	    ! dlnode_is_terminal(cursor);
	    cursor = dlnode_next(cursor)) {
		memcell_t *mc = (memcell_t *) cursor;
		printf("   boundary %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
	}

	for(cursor = dlist_first(s->reachable_listref);
	    ! dlnode_is_terminal(cursor);
	    cursor = dlnode_next(cursor)) {
		memcell_t *mc = (memcell_t *) cursor;
		printf("   reachable %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
	}

	for(cursor = dlist_first(s->unproc_listref);
	    ! dlnode_is_terminal(cursor);
	    cursor = dlnode_next(cursor)) {
		memcell_t *mc = (memcell_t *) cursor;
		printf("   unprocessed %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
	}

	for(cursor = dlist_first(&(s->free_list));
	    ! dlnode_is_terminal(cursor);
	    cursor = dlnode_next(cursor)) {
		memcell_t *mc = (memcell_t *) cursor;
		printf("   free: %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
	}
}
