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
	dlist_init(&(s->free_pending_list));
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
		assert(! mc->refcount);
		s->total_free--;
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): reuse node %p\n", s->iter_count, mc);
#endif
	} else {
		mc = (memcell_t *) dlnode_init(malloc(sizeof(memcell_t)+s->datasize));
		mc->locked = false;
		mc->refcount = 0;
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
	if(mc->refcount) {
		dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	} else {
		dlist_insertlast(&(s->free_pending_list), &(mc->hdr));
	}
	mc->locked = false;
}

void memory_gc_advise_new_link(memory_state_t *s, void *data)
{
	memcell_t *mc = data_to_memcell(data);
	mc->refcount++;
	if(!mc->locked && dlnode_owner(&(mc->hdr)) == &(s->roots_list)) {
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	}
}

void memory_gc_advise_stale_link(memory_state_t *s, void *data)
{
	memcell_t *mc = data_to_memcell(data);
	assert(mc->refcount > 0);
	mc->refcount --;
	if(!mc->refcount && !mc->locked) {
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(&(s->free_pending_list), &(mc->hdr));
	}
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

static void dl_cb_try_move_boundary(void *link, void *p)
{
	memory_state_t *s = (memory_state_t *) p;
	memcell_t *mc;

	if(!link) {
#if defined(GC_TRACING)
		printf("gc: move boundary null (noop)\n");
#endif
		return;
	}
	mc = data_to_memcell(link);

	/* linked data should never be in the free or free_pending list */
	assert(dlnode_owner(&(mc->hdr)) != &(s->free_list));
	assert(dlnode_owner(&(mc->hdr)) != &(s->free_pending_list));
	/* linked data should be referenced */
	assert(mc->refcount);

	if(dlnode_owner(&(mc->hdr)) == s->unproc_listref) {
#if defined(GC_TRACING)
		printf("gc: move boundary unproc %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	} else if(dlnode_owner(&(mc->hdr)) == &(s->roots_list)) {
#if defined(GC_TRACING)
		printf("gc: move boundary root %p%s", mc, mc->locked ? " (L)" : " ");
		s->p_cb(mc->data);
		printf("\n");
#endif
		if(!mc->locked) {
			dlnode_remove(&(mc->hdr));
			dlist_insertlast(&(s->boundary_list), &(mc->hdr));
		}
	} else {
#if defined(GC_TRACING)
		printf("gc: move boundary noop %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
	}
}

static void dl_cb_decref_free_pending_z(void *link, void *p)
{
	memory_state_t *s = (memory_state_t *) p;
	memcell_t *mc = data_to_memcell(link);

	assert(mc->refcount > 0);
	mc->refcount--;
	if(!mc->refcount) {
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(&(s->free_pending_list), &(mc->hdr));
	}
}

/* returns true when a complete gc cycle has been completed */
bool memory_gc_iterate(memory_state_t *s)
{
	memcell_t *mc;
	bool status = false;

	s->iter_count++;

	/* process free_pending nodes */
	if(! dlist_is_empty(&(s->free_pending_list))) {
		mc = (memcell_t *) dlist_first(&(s->free_pending_list));
		assert(!mc->locked); // locked nodes should stay in root list
		assert(!mc->refcount); // free_pending nodees should not be linked to
#if defined(GC_TRACING)
		printf("gc (%llu) iter free_pending: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): free node (pending) %p\n", s->iter_count, mc);
#endif
		s->dl_cb(dl_cb_decref_free_pending_z, &(mc->data), s);
		dlist_insertlast(&(s->free_list), dlnode_remove(&(mc->hdr)));
		s->total_free++;
		goto finish;
	}

	/* process boundary nodes */
	if(! dlist_is_empty(&(s->boundary_list))) {
		mc = (memcell_t *) dlist_first(&(s->boundary_list));
		assert(!mc->locked); // locked nodes should stay in root list
		assert(mc->refcount); // referenced nodes should have refcount
#if defined(GC_TRACING)
		printf("gc (%llu) iter reachable: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
		s->dl_cb(dl_cb_try_move_boundary, &(mc->data), s);
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(s->reachable_listref, &(mc->hdr));
		goto finish;
	}

	/* check to see if there are any root nodes to process */
	if(dlist_first(&(s->roots_list)) != &(s->root_sentinel)) {
		mc = (memcell_t *) dlist_first(&(s->roots_list));
#if defined(GC_TRACING)
		printf("gc (%llu) iter root: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
		s->dl_cb(dl_cb_try_move_boundary, &(mc->data), s);
		/* after processing, rotate to end of list */
		dlist_insertlast(&(s->roots_list), dlnode_remove(&(mc->hdr)));
		goto finish;
	}

	/* process unreachables */
	if(! dlist_is_empty(s->unproc_listref)) {
		mc = (memcell_t *) dlist_first(s->unproc_listref);
		assert(!mc->locked); // locked nodes should stay in root list
		assert(!mc->refcount); // unreachable should not be referenced
#if defined(GC_TRACING)
		printf("gc (%llu) iter unreachable: %p\n", s->iter_count, mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): free node (unreachable) %p\n", s->iter_count, mc);
#endif
		s->dl_cb(dl_cb_decref_free_pending_z, &(mc->data), s);
		dlist_insertlast(&(s->free_list), dlnode_remove(&(mc->hdr)));
		s->total_free++;
		goto finish;
	}

	/* reset state */
#if defined(GC_TRACING)
	printf("gc iter ** done **: reset state\n");
#endif
	assert(dlist_first(&(s->roots_list)) == &(s->root_sentinel));
	dlist_insertlast(&(s->roots_list), dlnode_remove(dlist_first(&(s->roots_list))));
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
	memcell_t *mc;

	printf("gc (%llu) state %p (%lu alloc %lu free %lu bytes total):\n",
	       s->iter_count, s,
	       (unsigned long) s->total_alloc, (unsigned long) s->total_free,
	       (unsigned long) ((sizeof(memcell_t)+s->datasize)*s->total_alloc));

	DLIST_FOR_FWD(&(s->free_pending_list), cursor) {
		mc = (memcell_t *) cursor;
		printf("   free_pend: %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
	}

	DLIST_FOR_FWD(&(s->roots_list), cursor) {
		if(cursor == &(s->root_sentinel)) {
			printf("   root (sentinel)\n");
		} else {
			mc = (memcell_t *) cursor;
			printf("   root %p ", mc);
			s->p_cb(mc->data);
			printf("\n");
		}

	}

	DLIST_FOR_FWD(&(s->boundary_list), cursor) {
		mc = (memcell_t *) cursor;
		printf("   boundary %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
	}

	DLIST_FOR_FWD(s->reachable_listref, cursor) {
		mc = (memcell_t *) cursor;
		printf("   reachable %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
	}

	DLIST_FOR_FWD(s->unproc_listref, cursor) {
		mc = (memcell_t *) cursor;
		printf("   unprocessed %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
	}

	DLIST_FOR_FWD(&(s->free_list), cursor) {
		mc = (memcell_t *) cursor;
		printf("   free: %p ", mc);
		s->p_cb(mc->data);
		printf("\n");
	}
}
