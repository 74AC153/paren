#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#if defined(CLEAR_ON_FREE)
#include "string.h"
#endif

#include "memory.h"

#define MS_FLAG_ACTIVE 0x1

#define MC_FLAG_LOCKED 0x1
#define MC_FLAG_SEARCHED 0x2

void memstate_print(memory_state_t *state)
{
	printf("memstate @ %p:\n", state);
	printf("datasize=%llu\n", (unsigned long long) state->datasize);
	printf("total_alloc=%llu\n", (unsigned long long) state->total_alloc);
	printf("total_free=%llu\n", (unsigned long long) state->total_free);
	printf("iter count=%llu\n", state->iter_count);
	printf("cycle count=%llu\n", state->cycle_count);
	printf("free list @ %p\n", &(state->free_list));
	printf("root sentinel @ %p\n", &(state->root_sentinel));
	printf("roots_list @ %p\n", &(state->roots_list));
	printf("boundary_list @ %p\n", &(state->boundary_list));
	printf("free_pending_list @ %p\n", &(state->free_pending_list));
	printf("white_list @ %p\n", &(state->white_list));
	printf("black_list @ %p\n", &(state->black_list));
	printf("reachable_listref: %p\n", state->reachable_listref);
	printf("unproc_listref: %p\n", state->unproc_listref);
	printf("init callback: %p\n", state->i_cb);
	printf("data_link_callback: %p\n", state->dl_cb);
	printf("print_callback: %p\n", state->p_cb);
}

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
	return (mc->mc_flags & MC_FLAG_LOCKED) != 0;
}

static
void memcell_lock(memcell_t *mc)
{
	mc->mc_flags |= MC_FLAG_LOCKED;
}

static
void memcell_unlock(memcell_t *mc)
{
	mc->mc_flags &= ~MC_FLAG_LOCKED;
}

static
void memcell_to_roots(memory_state_t *s, memcell_t *mc)
{
	dlist_insertlast(&(s->roots_list), &(mc->hdr));
}

static
void memcell_to_boundary(memory_state_t *s, memcell_t *mc)
{
	dlist_insertlast(&(s->boundary_list), &(mc->hdr));
}

static
void memcell_to_free_pending(memory_state_t *s, memcell_t *mc)
{
	dlist_insertlast(&(s->free_pending_list), &(mc->hdr));
}

static
void memcell_to_free(memory_state_t *s, memcell_t *mc)
{
	dlist_insertlast(&(s->free_list), &(mc->hdr));
}

static
void memcell_to_reachable(memory_state_t *s, memcell_t *mc)
{
	dlist_insertlast(s->reachable_listref, &(mc->hdr));
}



void memory_state_init(
	memory_state_t *s,
	size_t datasize,
	init_callback i_cb,
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
	s->i_cb = i_cb;
	s->dl_cb = dl_cb;
	s->p_cb = p_cb;
	s->total_alloc = 0;
	s->total_free = 0;
	s->iter_count = 0;
	s->cycle_count = 0;
	s->ms_flags = 0;
}

static
void memstate_setactive(memory_state_t *s)
{
	s->ms_flags |= MS_FLAG_ACTIVE;
}

static
void memstate_resetactive(memory_state_t *s)
{
	s->ms_flags &= ~MS_FLAG_ACTIVE;
}

static bool memstate_isactive(memory_state_t *s)
{
	return (s->ms_flags & MS_FLAG_ACTIVE) != 0;
}

void *memory_request(memory_state_t *s)
{
	memcell_t *mc;
	if(! dlist_is_empty(&(s->free_list))) {
		mc = (memcell_t *) dlnode_remove(dlist_first(&(s->free_list)));
		assert(! memcell_locked(mc));
		//assert(! memcell_refcount(mc));
#if ! defined(NO_GC_STATISTICS)
		assert(s->total_free);
		s->total_free--;
#endif
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): reuse node %p (%p)\n", s->iter_count, mc, mc->data);
#endif
	} else {
		mc = (memcell_t *) dlnode_init(malloc(sizeof(memcell_t)+s->datasize));
		memcell_unlock(mc);
		memcell_resetref(mc);
		memory_set_finalizer(mc->data, NULL);
#if ! defined(NO_GC_STATISTICS)
		s->total_alloc++;
#endif
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): malloc node %p (%p)\n", s->iter_count, mc, mc->data);
#endif
	}
	s->i_cb(mc->data);
	memcell_to_roots(s, mc);
	//dlist_insertlast(&(s->roots_list), &(mc->hdr));
	return mc->data;
}

static void memcell_deinit(memory_state_t *s, memcell_t *mc)
{
#if ! defined(NO_CLEAR_ON_FREE)
	memset(mc->data, 0, s->datasize);
#endif
	if(mc->fin) {
		mc->fin(mc->data);
	}
}

void memory_set_finalizer(void *data, data_fin_t fin)
{
	memcell_t *mc = data_to_memcell(data);
	
	mc->fin = fin;
}

bool memory_gc_is_locked(void *data)
{
	if(data) {
		return memcell_locked(data_to_memcell(data));
	} else {
		return false;
	}
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
#endif
		dlnode_remove(&(mc->hdr));
		dlnode_insertprev(&(s->root_sentinel), &(mc->hdr));
	} else {
#if defined(GC_TRACING)
		printf("gc: add root %p (noop) ", mc);
		s->p_cb(mc->data);
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
#endif
	dlnode_remove(&(mc->hdr));
	if(memcell_refcount(mc)) {
		memcell_to_boundary(s, mc);
		//dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	} else {
		memcell_to_free_pending(s, mc);
		//dlist_insertlast(&(s->free_pending_list), &(mc->hdr));
	}
	memcell_unlock(mc);
}

void memory_gc_advise_new_link(memory_state_t *s, void *data)
{
	/* increase refcount, move to 'boundary' if 'unprocessed' */

	memcell_t *mc;

	(void) s;
	if(!data) {
		return;
	}
	mc = data_to_memcell(data);
	memcell_incref(mc);
#if defined(GC_REFCOUNT_DEBUG)
	printf("gc: %p (%p) refcount++ -> %u\n",
	       mc, mc->data, (unsigned) memcell_refcount(mc));
#endif
	assert(dlnode_owner(&(mc->hdr)) != &(s->free_list));
	assert(dlnode_owner(&(mc->hdr)) != &(s->free_pending_list));
	/* pull this node out of the unprocessed list if applicable.
	   'boundaries' and 'reachables' are self-evidently unnecessary
	   'roots' will be taken care of eventually during GC iteration
	   if we don't do this, we may lose an 'unprocessed' node if:
	   - it is being linked to by a root node that has already been 'processed'
	   - and it is being unlnked from a nonroot that is not 'processed' */
	if(dlnode_owner(&(mc->hdr)) == s->unproc_listref) {
		dlnode_remove(&(mc->hdr));
		memcell_to_boundary(s, mc);
		//dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	}
}

void memory_gc_advise_stale_link(memory_state_t *s, void *data)
{
	memcell_t *mc;

	/* decrease refcount, move to free_pending unreferenced and not locked */

	if(!data) {
		return;
	}
	mc = data_to_memcell(data);
	if(! memcell_refcount(mc)) { // may already be zero if there is a loop
#if defined(GC_REFCOUNT_DEBUG)
		printf("gc: %p (%p) refcount-- 0 -> 0 (loop?)\n", mc, mc->data);
#endif
		/* unreferenced nodes shouldn't be live */
		assert(dlnode_owner(&(mc->hdr)) == &(s->free_pending_list) ||
		       dlnode_owner(&(mc->hdr)) == &(s->free_list));
		return;
	}
	memcell_decref(mc);
#if defined(GC_REFCOUNT_DEBUG)
	printf("gc: %p (%p) refcount-- -> %u\n",
	       mc, mc->data, (unsigned) memcell_refcount(mc));
#endif
	if(! memcell_refcount(mc) && ! memcell_locked(mc)) {
		dlnode_remove(&(mc->hdr));
		memcell_to_free_pending(s, mc);
		//dlist_insertlast(&(s->free_pending_list), &(mc->hdr));
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
#endif
		dlnode_remove(&(mc->hdr));
		memcell_to_boundary(s, mc);
		//dlist_insertlast(&(s->boundary_list), &(mc->hdr));
	} else if(dlnode_owner(&(mc->hdr)) == &(s->roots_list)) {
#if defined(GC_TRACING)
		printf("gc: move boundary root %p%s",
		       mc, memcell_locked(mc) ? " (L)" : " ");
		s->p_cb(mc->data);
#endif
		if(! memcell_locked(mc)) {
			dlnode_remove(&(mc->hdr));
			memcell_to_boundary(s, mc);
			//dlist_insertlast(&(s->boundary_list), &(mc->hdr));
		}
	} else {
#if defined(GC_TRACING)
		printf("gc: move boundary noop %p ", mc);
		s->p_cb(mc->data);
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

	assert(!memstate_isactive(s));
#if !defined(NDEBUG) /* this is useless without the assert above */
	memstate_setactive(s);
#endif

#if ! defined(NO_GC_STATISTICS)
	s->iter_count++;
#endif

	/* process free_pending nodes: move them to free_list */
	if(! dlist_is_empty(&(s->free_pending_list))) {
		mc = (memcell_t *) dlist_first(&(s->free_pending_list));
#if defined(GC_REACHABILITY_VERIFICATION)
		assert(!memcell_reachable(s, mc));
#endif
#if defined(GC_TRACING)
		printf("gc (%llu) iter free_pending: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
#endif
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): free node (pending) %p\n", s->iter_count, mc);
#endif
		assert(!memcell_locked(mc)); // locked nodes must stay in root list
		/* TODO: what about loops here? should they still be unlinked? */
		assert(!memcell_refcount(mc)); // free_pending nodees must be unlinked
		s->dl_cb(dl_cb_decref_free_pending_z, &(mc->data), s);
		memcell_deinit(s, mc);
		dlnode_remove(&(mc->hdr));
		memcell_to_free(s, mc);
		//dlist_insertlast(&(s->free_list), dlnode_remove(&(mc->hdr)));
#if ! defined(NO_GC_STATISTICS)
		s->total_free++;
		assert(s->total_free <= s->total_alloc);
#endif
		goto finish;
	}

	/* process boundary nodes: move them to reachable */
	if(! dlist_is_empty(&(s->boundary_list))) {
		mc = (memcell_t *) dlist_first(&(s->boundary_list));
#if defined(GC_TRACING)
		printf("gc (%llu) iter reachable: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
#endif
		assert(!memcell_locked(mc)); // locked nodes should stay in root list
		assert(memcell_refcount(mc)); // referenced nodes should have refcount
		s->dl_cb(dl_cb_try_move_boundary, &(mc->data), s);
		dlnode_remove(&(mc->hdr));
		memcell_to_reachable(s, mc);
		//dlist_insertlast(s->reachable_listref, dlnode_remove(&(mc->hdr)));
		goto finish;
	}

	/* check to see if there are any root nodes to process: move their links
	   to boundary */
	if(dlist_first(&(s->roots_list)) != &(s->root_sentinel)) {
		mc = (memcell_t *) dlist_first(&(s->roots_list));
#if defined(GC_TRACING)
		printf("gc (%llu) iter root: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
#endif
		s->dl_cb(dl_cb_try_move_boundary, &(mc->data), s);
		/* after processing, rotate to end of list */
		dlnode_remove(&(mc->hdr));
		memcell_to_roots(s, mc);
		//dlist_insertlast(&(s->roots_list), dlnode_remove(&(mc->hdr)));
		goto finish;
	}

	/* remaining 'unprocessed' nodes are unreachable: 'free' them */
	if(! dlist_is_empty(s->unproc_listref)) {
		mc = (memcell_t *) dlist_first(s->unproc_listref);
#if defined(GC_REACHABILITY_VERIFICATION)
		assert(!memcell_reachable(s, mc));
#endif
#if defined(GC_TRACING)
		printf("gc (%llu) iter unreachable: %p ", s->iter_count, mc);
		s->p_cb(mc->data);
#endif
#if defined(ALLOC_DEBUG)
		printf("gc (%llu): free node (unreachable) %p ", s->iter_count, mc);
#endif
		assert(! memcell_locked(mc)); // locked nodes should stay in root list
		// NB: unreachable can be referenced if e.g. lambda points back to it.
		s->dl_cb(dl_cb_decref_free_pending_z, &(mc->data), s);
		memcell_deinit(s, mc);
		dlnode_remove(&(mc->hdr));
		memcell_to_free(s, mc);
		//dlist_insertlast(&(s->free_list), dlnode_remove(&(mc->hdr)));
#if ! defined(NO_GC_STATISTICS)
		s->total_free++;
		assert(s->total_free <= s->total_alloc);
#endif
		goto finish;
	}

	/* reset state -- progress root_sentinel, swap reachable/unproc lists */
#if defined(GC_TRACING)
	printf("gc iter ** done **: reset state\n");
#endif
	assert(dlist_first(&(s->roots_list)) == &(s->root_sentinel));
	mc = (memcell_t *) dlnode_remove(dlist_first(&(s->roots_list)));
	memcell_to_roots(s, mc);
	//dlist_insertlast(&(s->roots_list),
	//                 dlnode_remove(dlist_first(&(s->roots_list))));
	if(s->reachable_listref == &(s->white_list)) {
		s->unproc_listref = s->reachable_listref;
		s->reachable_listref = &(s->black_list);
	} else {
		s->unproc_listref = s->reachable_listref;
		s->reachable_listref = &(s->white_list);
	}
	status = true;
#if ! defined(NO_GC_STATISTICS)
	s->cycle_count++;
#endif

finish:
#if defined(GC_VERBOSE)
	memory_gc_print_state(s);
#endif
#if !defined(NDEBUG) /* useless without the non-recursive assertion above */
	memstate_resetactive(s);
#endif
	return status;
}


/* non-essential functions */

struct _reach_info_
{
	bool found;
	memcell_t *dest;
	memory_state_t *s;
};

static void reachable_helper(void *link, void *p)
{
	struct _reach_info_ *ri = (struct _reach_info_ *) p;
	memcell_t *mc;

	if(!link) return;

	mc = data_to_memcell(link);

	/* recursive depth-first search with loop detection */

	if(mc->mc_flags & MC_FLAG_SEARCHED) {
		return;
	}

	mc->mc_flags |= MC_FLAG_SEARCHED;

	if(mc == ri->dest) {
		ri->found = true;
	} else {
		ri->s->dl_cb(reachable_helper, &(mc->data), ri);
	}

	mc->mc_flags &= ~MC_FLAG_SEARCHED;
}

bool memcell_reachable(memory_state_t *s, memcell_t *dst)
{
	struct _reach_info_ ri;
	dlnode_t *cursor;

	ri.found = false;
	ri.dest = dst;
	ri.s = s;

	/* see if dst is reachable from any of the root nodes */
	DLIST_FOR_FWD(&(s->roots_list), cursor) {
		if(cursor == &(s->root_sentinel)) {
			/* skip root sentinel */
			continue;
		}
		reachable_helper( &(((memcell_t *) cursor)->data), &ri);
		if(ri.found) {
			break;
		}
	}

	return ri.found;
}

static void memcell_print_meta(memory_state_t *s, memcell_t *mc)
{
	char *listname = NULL;
	unsigned long long refcount;
	bool reachable, locked;

	if(mc->hdr.owner == &(s->free_list)) listname = "free";
	else if(mc->hdr.owner == &(s->roots_list)) listname = "root";
	else if(mc->hdr.owner == &(s->boundary_list) )listname = "boundary";
	else if(mc->hdr.owner == &(s->free_pending_list) )listname = "free_pend";
	else if(mc->hdr.owner == s->reachable_listref) listname = "reachable";
	else if(mc->hdr.owner == s->unproc_listref) listname = "unproc";
	else assert(false);

	refcount = memcell_refcount(mc);
	reachable = memcell_reachable(s, mc);
	locked = memcell_locked(mc);

	printf("%s %p refcount=%llu reachable=%s locked=%s ",
	       listname, mc,
	       refcount,
	       reachable ? "y" : "n",
	       locked ? "y": "n");
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
		memcell_print_meta(s, mc);
		s->p_cb(mc->data);
	}

	DLIST_FOR_FWD(&(s->roots_list), cursor) {
		if(cursor == &(s->root_sentinel)) {
			printf("root sentinel %p\n", cursor);
		} else {
			mc = (memcell_t *) cursor;
			memcell_print_meta(s, mc);
			s->p_cb(mc->data);
		}
	}

	DLIST_FOR_FWD(&(s->boundary_list), cursor) {
		mc = (memcell_t *) cursor;
		memcell_print_meta(s, mc);
		s->p_cb(mc->data);
	}

	DLIST_FOR_FWD(s->reachable_listref, cursor) {
		mc = (memcell_t *) cursor;
		memcell_print_meta(s, mc);
		s->p_cb(mc->data);
	}

	DLIST_FOR_FWD(s->unproc_listref, cursor) {
		mc = (memcell_t *) cursor;
		memcell_print_meta(s, mc);
		s->p_cb(mc->data);
	}

	DLIST_FOR_FWD(&(s->free_list), cursor) {
		mc = (memcell_t *) cursor;
		memcell_print_meta(s, mc);
		s->p_cb(mc->data);
	}
}

unsigned long long memory_gc_count_iters(memory_state_t *s)
{
	return s->iter_count;
}

unsigned long long memory_gc_count_cycles(memory_state_t *s)
{
	return s->cycle_count;
}

uintptr_t memory_gc_count_total(memory_state_t *s)
{
	return s->total_alloc;
}

uintptr_t memory_gc_count_free(memory_state_t *s)
{
	return s->total_free;
}
