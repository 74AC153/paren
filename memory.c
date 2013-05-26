#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#include "memory.h"

static
memcell_t *data_to_memcell(void *data)
{
	return (memcell_t *) (data - offsetof(memcell_t, data));
}

static
uintptr_t memcell_refcount(memcell_t *mc)
{
	return mc->refcount;
}

static
void memcell_incref(memcell_t *mc)
{
	mc->refcount++;
}

static
void memcell_decref(memcell_t *mc)
{
	mc->refcount--;
}

static
void memcell_resetref(memcell_t *mc)
{
	mc->refcount = 0;
}

static
bool memcell_locked(memcell_t *mc)
{
	return mc->locked;
}

static
void memcell_lock(memcell_t *mc)
{
	mc->locked = true;
}

static
void memcell_unlock(memcell_t *mc)
{
	mc->locked = false;
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
	s->iter_count = 0;
}

void *memory_request(memory_state_t *s)
{
	memcell_t *mc;
	if(! dlist_is_empty(&(s->free_list))) {
		mc = (memcell_t *) dlnode_remove(dlist_first(&(s->free_list)));
		assert(! memcell_locked(mc));
		assert(! memcell_refcount(mc));
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): reuse node %p (%p)\n", s->iter_count, mc, mc->data);
#endif
	} else {
		mc = (memcell_t *) dlnode_init(malloc(sizeof(memcell_t)+s->datasize));
		memcell_unlock(mc);
		memcell_resetref(mc);
		s->total_alloc++;
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): malloc node %p (%p)\n", s->iter_count, mc, mc->data);
#endif
	}
	dlist_insertlast(&(s->roots_list), &(mc->hdr));
	return mc->data;
}

void memory_gc_lock(memory_state_t *s, void *data)
{
	/* set locked state and make root */

	memcell_t *mc = data_to_memcell(data);
	memcell_lock(mc);
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
	/* clear lock, make boundary if referenced, else make free_pending */

	memcell_t *mc = data_to_memcell(data);
	assert(dlnode_owner(&(mc->hdr)) == &(s->roots_list));
#if defined(GC_TRACING)
	printf("gc: rem root %p ", mc);
	s->p_cb(mc->data);
	printf("\n");
#endif
	dlnode_remove(&(mc->hdr));
	if(memcell_refcount(mc)) {
		dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	} else {
		dlist_insertlast(&(s->free_pending_list), &(mc->hdr));
	}
	memcell_unlock(mc);
}

void memory_gc_advise_new_link(memory_state_t *s, void *data)
{
	memcell_t *mc;
	/* increase refcount, and add to boundary if unlocked root */
	if(!data) {
		return;
	}
	mc = data_to_memcell(data);
	memcell_incref(mc);
#if defined(GC_REFCOUNT_DEBUG)
	printf("gc: %p (%p) refcount++ -> %u\n",
	       mc, mc->data, (unsigned) memcell_refcount(mc));
#endif
	if((! memcell_locked(mc) && dlnode_owner(&(mc->hdr)) == &(s->roots_list))
	   || dlnode_owner(&(mc->hdr)) == s->unproc_listref) {
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	}
}

void memory_gc_advise_stale_link(memory_state_t *s, void *data)
{
	memcell_t *mc;
	/* decrease refcount and add to free_pending if not locked */
	if(!data) {
		return;
	}
	mc = data_to_memcell(data);
	if(! memcell_refcount(mc)) { // may already be zero if there is a loop
#if defined(GC_REFCOUNT_DEBUG)
		printf("gc: %p (%p) refcount-- 0 -> 0 (loop?)\n", mc, mc->data);
#endif
		return;
	}
	memcell_decref(mc);
#if defined(GC_REFCOUNT_DEBUG)
	printf("gc: %p (%p) refcount-- -> %u\n",
	       mc, mc->data, (unsigned) memcell_refcount(mc));
#endif
	if(! memcell_refcount(mc) && ! memcell_locked(mc)) {
		dlnode_remove(&(mc->hdr));
		dlist_insertlast(&(s->free_pending_list), &(mc->hdr));
	}
}

bool memory_gc_isroot(memory_state_t *s, void *data)
{
	bool status;
	memcell_t *mc;
	if(!data) {
		return false;
	}
	mc = data_to_memcell(data);
	status = dlnode_owner(&(mc->hdr)) == &(s->roots_list);
	return status;
}

bool memory_gc_islive(memory_state_t *s, void *data)
{
	memcell_t *mc;
	if(!data) {
		return false;
	}
	mc = data_to_memcell(data);
	return dlnode_owner(&(mc->hdr)) != &(s->free_list);
}

static void dl_cb_try_move_boundary(void *link, void *p)
{
	/* move to boundary if unprocessed, or an unlocked root */

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
	assert(memcell_refcount(mc));

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
		printf("gc: move boundary root %p%s",
		       mc, memcell_locked(mc) ? " (L)" : " ");
		s->p_cb(mc->data);
		printf("\n");
#endif
		if(! memcell_locked(mc)) {
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
	memory_gc_advise_stale_link(s, link);
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
#if defined(GC_TRACING)
		printf("gc (%llu) iter free_pending: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): free node (pending) %p\n", s->iter_count, mc);
#endif
		assert(!memcell_locked(mc)); // locked nodes must stay in root list
		assert(!memcell_refcount(mc)); // free_pending nodees must be unlinked
		s->dl_cb(dl_cb_decref_free_pending_z, &(mc->data), s);
		dlist_insertlast(&(s->free_list), dlnode_remove(&(mc->hdr)));
		goto finish;
	}

	/* process boundary nodes */
	if(! dlist_is_empty(&(s->boundary_list))) {
		mc = (memcell_t *) dlist_first(&(s->boundary_list));
#if defined(GC_TRACING)
		printf("gc (%llu) iter reachable: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
		assert(!memcell_locked(mc)); // locked nodes should stay in root list
		assert(memcell_refcount(mc)); // referenced nodes should have refcount
		s->dl_cb(dl_cb_try_move_boundary, &(mc->data), s);
		dlist_insertlast(s->reachable_listref, dlnode_remove(&(mc->hdr)));
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
#if defined(GC_TRACING)
		printf("gc (%llu) iter unreachable: %p\n", s->iter_count, mc);
		s->p_cb(mc->data);
		printf("\n");
#endif
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): free node (unreachable) %p\n", s->iter_count, mc);
#endif
		assert(! memcell_locked(mc)); // locked nodes should stay in root list
		// NB: unreachable can be referenced if e.g. lambda points back to it.
		s->dl_cb(dl_cb_decref_free_pending_z, &(mc->data), s);
		dlist_insertlast(&(s->free_list), dlnode_remove(&(mc->hdr)));
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

	printf("gc (%llu) state %p (%lu alloc, %lu bytes):\n",
	       s->iter_count, s,
	       (unsigned long) s->total_alloc, 
	       (unsigned long) ((sizeof(memcell_t)+s->datasize)*s->total_alloc));

	DLIST_FOR_FWD(&(s->free_pending_list), cursor) {
		mc = (memcell_t *) cursor;
		printf("   free_pend: %p (%u) ", mc, (unsigned) memcell_refcount(mc));
		s->p_cb(mc->data);
		printf("\n");
	}

	DLIST_FOR_FWD(&(s->roots_list), cursor) {
		if(cursor == &(s->root_sentinel)) {
			printf("   root (sentinel)\n");
		} else {
			mc = (memcell_t *) cursor;
			printf("   root %p (%u) %s",
			       mc, (unsigned) memcell_refcount(mc),
			       memcell_locked(mc) ? "L ": "");
			s->p_cb(mc->data);
			printf("\n");
		}

	}

	DLIST_FOR_FWD(&(s->boundary_list), cursor) {
		mc = (memcell_t *) cursor;
		printf("   boundary %p (%u) ", mc, (unsigned) memcell_refcount(mc));
		s->p_cb(mc->data);
		printf("\n");
	}

	DLIST_FOR_FWD(s->reachable_listref, cursor) {
		mc = (memcell_t *) cursor;
		printf("   reachable %p (%u) ", mc, (unsigned) memcell_refcount(mc));
		s->p_cb(mc->data);
		printf("\n");
	}

	DLIST_FOR_FWD(s->unproc_listref, cursor) {
		mc = (memcell_t *) cursor;
		printf("   unprocessed %p (%u) ", mc, (unsigned) memcell_refcount(mc));
		s->p_cb(mc->data);
		printf("\n");
	}

	DLIST_FOR_FWD(&(s->free_list), cursor) {
		mc = (memcell_t *) cursor;
		printf("   free: %p (%u) ", mc, (unsigned) memcell_refcount(mc));
		s->p_cb(mc->data);
		printf("\n");
	}
}
