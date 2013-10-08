#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "stream.h"
#include "libc_custom.h"
#include "traceclass.h"
#include "dbgtrace.h"

#include "memory.h"

#define MS_FLAG_ACTIVE 0x1

void memstate_print(memory_state_t *state, stream_t *s)
{
	char b[17], b2[17];
	stream_putln(s, "memstate @ 0x", fmt_ptr(b, state), ":");
	stream_putln(s, "total_alloc=", fmt_s64(b2, state->total_alloc));
#if ! defined(NO_GC_FREELIST)
	stream_putln(s, "total_free=", fmt_s64(b2, state->total_free));
#endif /* ! defined(NO_GC_FREELIST) */
	stream_putln(s, "iter count=", fmt_s64(b2, state->iter_count));
	stream_putln(s, "cycle count=", fmt_s64(b2, state->cycle_count));
	stream_putln(s, "clean_cycles=", fmt_s64(b2, state->clean_cycles));
	stream_putln(s, "skipped_clean_iters=",
	                fmt_s64(b2, state->skipped_clean_iters));
#if ! defined(NO_GC_FREELIST)
	stream_putln(s, "free list @ 0x", fmt_ptr(b, &(state->free_list)));
#endif
	stream_putln(s, "root sentinel @ 0x", fmt_ptr(b, &(state->root_sentinel)));
	stream_putln(s, "roots_list @ 0x", fmt_ptr(b, &(state->roots_list)));
	stream_putln(s, "boundary_list @ 0x", fmt_ptr(b, &(state->boundary_list)));
	stream_putln(s, "free_pending_list @ 0x",
	                fmt_ptr(b, &(state->free_pending_list)));
	stream_putln(s, "white_list @ 0x", fmt_ptr(b, &(state->white_list)));
	stream_putln(s, "black_list @ 0x", fmt_ptr(b, &(state->black_list)));
	stream_putln(s, "reachable_listref: 0x",
	                fmt_ptr(b, state->reachable_listref));
	stream_putln(s, "unproc_listref: 0x", fmt_ptr(b, state->unproc_listref));
	stream_putln(s, "init callback: 0x", fmt_ptr(b, state->i_cb));
	stream_putln(s, "data_link_callback: 0x", fmt_ptr(b, state->dl_cb));
	stream_putln(s, "print_callback: 0x", fmt_ptr(b, state->p_cb));
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
	return mc->mc_flags.locked;
}

static
void memcell_lock(memcell_t *mc)
{
	mc->mc_flags.locked = true;
}

static
void memcell_unlock(memcell_t *mc)
{
	mc->mc_flags.locked = false;
}

static
bool memcell_is_root(memory_state_t *s, memcell_t *mc)
{
	return dlnode_owner(&(mc->hdr)) == &(s->roots_list);
}

static
bool memcell_is_free(memory_state_t *s, memcell_t *mc)
{
#if ! defined(NO_GC_FREELIST)
	assert(mc->mc_flags.live == (dlnode_owner(&(mc->hdr)) != &(s->free_list)));
#endif /* ! defined(NO_GC_FREELIST) */
	return ! mc->mc_flags.live;
}

static
bool memcell_is_free_pending(memory_state_t *s, memcell_t *mc)
{
	return dlnode_owner(&(mc->hdr)) == &(s->free_pending_list);
}

static
bool memcell_is_unproc(memory_state_t *s, memcell_t *mc)
{
	return dlnode_owner(&(mc->hdr)) == s->unproc_listref;
}

static
void memcell_to_roots_unproc(memory_state_t *s, memcell_t *mc)
{
	assert(&(mc->hdr) != &(s->root_sentinel));
	dlnode_insertprev(&(s->root_sentinel), &(mc->hdr));
}

static
void memcell_to_roots_proc(memory_state_t *s, memcell_t *mc)
{
	assert(&(mc->hdr) != &(s->root_sentinel));
	dlnode_insertnext(&(s->root_sentinel), &(mc->hdr));
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
void memcell_deinit(memory_state_t *s, memcell_t *mc)
{
	if(mc->fin) {
		mc->fin(mc->data);
	}
}

#if ! defined(NO_GC_FREELIST)
static
void memcell_free(memory_state_t *s, memcell_t *mc)
{
	memcell_deinit(s, mc);
	mc->mc_flags.live = false;
	dlnode_remove(&(mc->hdr));
	dlist_insertlast(&(s->free_list), &(mc->hdr));
#if ! defined(NO_GC_STATISTICS)
	s->total_free++;
#endif
}
#else /* defined(NO_GC_FREELIST) */
static
void memcell_free(memory_state_t *s, memcell_t *mc)
{
	memcell_deinit(s, mc);
	/* note that this is more 'hopeful' than useful, as we don't know what will
	   happen to the memory once we call mem_free(). However, if the memory
	   isn't immediately reused and we try to reference it, then this may help
	   track down errors */
	mc->mc_flags.live = false;
	dlnode_remove(&(mc->hdr));
	s->mem_free(mc, s->mem_alloc_priv);
	s->total_alloc--;
}
#endif /* ! defined(NO_GC_FREELIST) */

static
void memcell_to_reachable(memory_state_t *s, memcell_t *mc)
{
	dlist_insertlast(s->reachable_listref, &(mc->hdr));
}

void memory_state_init(
	memory_state_t *s,
	init_callback i_cb,
	data_link_callback dl_cb,
	print_callback p_cb,
	mem_allocator_fn_t mem_alloc,
	mem_free_fn_t mem_free,
	void *mem_alloc_priv)
{
#if ! defined(NO_GC_FREELIST)
	dlist_init(&(s->free_list));
#endif /* ! defined(NO_GC_FREELIST) */
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
#if ! defined(NO_GC_FREELIST)
	s->total_free = 0;
#endif /* ! defined(NO_GC_FREELIST) */
	s->iter_count = 0;
	s->cycle_count = 0;
	s->clean_cycles = 0;
	s->skipped_clean_iters = 0;
	s->ms_flags.active = false;
	s->mem_alloc = mem_alloc;
	s->mem_free = mem_free;
	s->mem_alloc_priv = mem_alloc_priv;
}

void memory_state_reset(
	memory_state_t *s)
{
	dlnode_t *cursor;
	memcell_t *mc;

	while(! dlist_is_empty(&(s->free_pending_list))) {
		mc = (memcell_t *) dlist_first(&(s->free_pending_list));
		memcell_free(s, mc);
	}

	while(! dlist_is_empty(&(s->roots_list))) {
		cursor = dlist_first(&(s->roots_list));
		if(cursor == &(s->root_sentinel)) {
			dlnode_remove(cursor);
		} else {
			mc = (memcell_t *) cursor;
			memcell_free(s, mc);
		}
	}

	while(! dlist_is_empty(&(s->boundary_list))) {
		mc = (memcell_t *) dlist_first(&(s->boundary_list));
		memcell_free(s, mc);
	}

	while(! dlist_is_empty(&(s->black_list))) {
		mc = (memcell_t *) dlist_first(&(s->black_list));
		memcell_free(s, mc);
	}

	while(! dlist_is_empty(&(s->white_list))) {
		mc = (memcell_t *) dlist_first(&(s->white_list));
		memcell_free(s, mc);
	}

#if ! defined(NO_GC_FREELIST)
	while(! dlist_is_empty(&(s->free_list))) {
		mc = (memcell_t *) dlnode_remove(dlist_first(&(s->free_list)));
		s->mem_free(mc, s->mem_alloc_priv);
	}
#endif
}

memory_state_t *data_to_memstate(void *data)
{
	memcell_t *mc = data_to_memcell(data);
	return mc->state;
}

static
void memstate_setactive(memory_state_t *s)
{
	s->ms_flags.active = true;
}

static
void memstate_resetactive(memory_state_t *s)
{
	s->ms_flags.active = false;
}

static bool memstate_isactive(memory_state_t *s)
{
	return s->ms_flags.active;
}

static void memcell_init(memory_state_t *s, memcell_t *mc)
{
	mc->mc_flags.live = true;
	mc->state = s;
	memcell_unlock(mc);
	memcell_resetref(mc);
	memory_set_finalizer(mc->data, NULL);
	s->i_cb(mc->data);
	memcell_to_roots_unproc(s, mc);
}

#if ! defined(NO_GC_FREELIST)
void *memory_request(memory_state_t *s, size_t len)
{
	memcell_t *mc;
	DBGSTMT(char buf[21]);
	DBGSTMT(char buf2[21]);
	DBGSTMT(char buf3[21]);
	DBGSTMT(char buf4[21]);

	if(! dlist_is_empty(&(s->free_list))) {
		mc = (memcell_t *) dlnode_remove(dlist_first(&(s->free_list)));
		assert(! memcell_locked(mc));
		//assert(! memcell_refcount(mc)); // why not enable?
#if ! defined(NO_GC_STATISTICS)
		assert(s->total_free);
		s->total_free--;
#endif /* ! defined(NO_GC_STATISTICS) */
		DBGTRACELN(TC_MEM_ALLOC,
		           "gc (", fmt_u64d(buf, s->iter_count), "): ",
		           "reuse node ", fmt_ptr(buf2, mc), " ",
		           "(", fmt_ptr(buf3, mc->data), ") ",
		           "nfree=", fmt_u64d(buf4, s->total_free));
	} else {
		void *mem = s->mem_alloc(sizeof(memcell_t) + len,
		                         s->mem_alloc_priv);
		mc = (memcell_t *) dlnode_init(mem);
#if ! defined(NO_GC_STATISTICS)
		s->total_alloc++;
#endif /* ! defined(NO_GC_STATISTICS) */
		DBGTRACELN(TC_MEM_ALLOC,
		           "gc (", fmt_u64d(buf, s->iter_count), "): ",
		           "malloc node ", fmt_ptr(buf2, mc), " ",
		           "(", fmt_ptr(buf3, mc->data), ") ",
		           "nalloc=", fmt_u64d(buf4, s->total_alloc));
	}

	memcell_init(s, mc);
	return mc->data;
}
#else /* defined(NO_GC_FREELIST) */
void *memory_request(memory_state_t *s, size_t len)
{
	memcell_t *mc;
	mc = s->mem_alloc(sizeof(memcell_t) + len, s->mem_alloc_priv);
	dlnode_init(&(mc->hdr));
	memcell_init(s, mc);
	s->total_alloc++;
	return mc->data;
}
#endif /* ! defined(NO_GC_FREELIST) */

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

	DBGSTMT(char buf[21];)

	memcell_t *mc = data_to_memcell(data);
	memcell_lock(mc);
	if(! memcell_is_root(s, mc)) {
		DBGTRACE(TC_GC_TRACING, "gc: add root ", fmt_ptr(buf, mc), " ");
		DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });
		dlnode_remove(&(mc->hdr));
		memcell_to_roots_unproc(s, mc);
	} else {
		DBGTRACE(TC_GC_TRACING, "gc: add root ", fmt_ptr(buf, mc), " (noop) ");
		DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });
	}
}

void memory_gc_unlock(memory_state_t *s, void *data)
{
	/* clear lock, make boundary if referenced, else make free_pending */
	DBGSTMT(char buf[21];);

	memcell_t *mc = data_to_memcell(data);
	assert(memcell_is_root(s, mc));

	DBGTRACE(TC_GC_TRACING, "gc: rem root ", fmt_ptr(buf, mc), " ");
	DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });

	dlnode_remove(&(mc->hdr));
	if(memcell_refcount(mc)) {
		memcell_to_boundary(s, mc);
	} else {
		memcell_to_free_pending(s, mc);
	}
	memcell_unlock(mc);
	s->clean_cycles = 0; // may need to begin cleanup
}

void memory_gc_advise_new_link(memory_state_t *s, void *data)
{
	/* increase refcount, move to 'boundary' if 'unprocessed' */

	memcell_t *mc;
	DBGSTMT(char buf[21]);
	DBGSTMT(char buf2[21]);
	DBGSTMT(char buf3[21]);

	(void) s;
	if(!data) {
		return;
	}
	mc = data_to_memcell(data);
	memcell_incref(mc);
	DBGTRACELN(TC_MEM_RC,
	           "gc: ", fmt_ptr(buf, mc), " ",
	           "(", fmt_ptr(buf2, mc->data), ") ",
	           "refcount++ -> ", fmt_u64d(buf3, memcell_refcount(mc)));
	assert(! memcell_is_free(s, mc));
	assert(! memcell_is_free_pending(s, mc));
	/* pull this node out of the unprocessed list if applicable.
	   'boundaries' and 'reachables' are self-evidently unnecessary
	   'roots' will be taken care of eventually during GC iteration
	   if we don't do this, we may lose an 'unprocessed' node if:
	   - it is being linked to by a root node that has already been 'processed'
	   - and it is being unlnked from a nonroot that is not 'processed' */
	if(memcell_is_unproc(s, mc)) {
		dlnode_remove(&(mc->hdr));
		memcell_to_boundary(s, mc);
	}
}

void memory_gc_advise_stale_link(memory_state_t *s, void *data)
{
	memcell_t *mc;
	DBGSTMT(char buf[21]);
	DBGSTMT(char buf2[21]);
	DBGSTMT(char buf3[21]);

	/* decrease refcount, move to free_pending unreferenced and not locked */

	if(!data) {
		return;
	}
	s->clean_cycles = 0; // may need to begin cleanup
	mc = data_to_memcell(data);
	if(! memcell_refcount(mc)) { // may already be zero if there is a loop
		DBGTRACELN(TC_MEM_RC,
		           "gc: ", fmt_ptr(buf, mc), " ",
		           "(", fmt_ptr(buf2, mc->data), ") ",
		           "refcount-- 0 -> 0 (loop?)");
		/* unreferenced nodes shouldn't be live */
		assert(memcell_is_free_pending(s, mc) ||
		       memcell_is_free(s, mc));
		return;
	}
	memcell_decref(mc);
	DBGTRACELN(TC_MEM_RC,
	           "gc: ", fmt_ptr(buf, mc), " ",
	           "(", fmt_ptr(buf2, mc->data), ") ",
	           "refcount-- -> ", fmt_u64d(buf3, memcell_refcount(mc)));
	if(! memcell_refcount(mc)
	   && ! memcell_locked(mc)
	   && ! memcell_is_free(s, mc) /* if loop node may already be in free */) {
		dlnode_remove(&(mc->hdr));
		memcell_to_free_pending(s, mc);
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
	status = memcell_is_root(s, mc);
	return status;
}

bool memory_gc_islive(memory_state_t *s, void *data)
{
	memcell_t *mc;
	if(!data) {
		return false;
	}
	mc = data_to_memcell(data);
	return !memcell_is_free(s, mc);
}

static void dl_cb_try_move_boundary(void *link, void *p)
{
	/* move to boundary if unprocessed, or an unlocked root */
	DBGSTMT(char buf[21];)

	memory_state_t *s = (memory_state_t *) p;
	memcell_t *mc;

	if(!link) {
		DBGTRACELN(TC_GC_TRACING,"gc: move boundary null (noop)");
		return;
	}
	mc = data_to_memcell(link);

	/* linked data should never be in the free or free_pending list */
	assert(! memcell_is_free(s, mc));
	assert(! memcell_is_free_pending(s, mc));
	/* linked data should be referenced */
	assert(memcell_refcount(mc));

	if(memcell_is_unproc(s, mc)){
		DBGTRACE(TC_GC_TRACING,
		         "gc: move boundary unproc ", fmt_ptr(buf, mc), " ");
		DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });
		dlnode_remove(&(mc->hdr));
		memcell_to_boundary(s, mc);
	} else if(memcell_is_root(s, mc)) {
		DBGTRACE(TC_GC_TRACING,
		         "gc: move boundary root ", fmt_ptr(buf, mc), 
		         (memcell_locked(mc) ? " (L)" : " "));
		DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });
		if(! memcell_locked(mc)) {
			dlnode_remove(&(mc->hdr));
			memcell_to_boundary(s, mc);
		}
	} else {
		DBGTRACE(TC_GC_TRACING,
		         "gc: move boundary noop ", fmt_ptr(buf, mc), " ");
		DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });
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
	DBGSTMT(char buf[21]);
	DBGSTMT(char buf2[21]);
	DBGSTMT(char buf3[21]);
	memcell_t *mc;
	bool status = false;

	assert(!memstate_isactive(s));
#if !defined(NDEBUG) /* this is useless without the assert above */
	memstate_setactive(s);
#endif
	/* 2 clean cycles allows unprocessed nodes to reach the free list */
	if(s->clean_cycles == 2) {
		s->skipped_clean_iters++;
		/* although we didn't cycle, we return true because the last
		   "full" cycle was clean */
		status = true;
		goto finish;
	}

#if ! defined(NO_GC_STATISTICS)
	s->iter_count++;
#endif

	/* process free_pending nodes: move them to free_list */
	if(! dlist_is_empty(&(s->free_pending_list))) {
		mc = (memcell_t *) dlist_first(&(s->free_pending_list));
#if defined(GC_REACHABILITY_VERIFICATION)
		assert(!memcell_reachable(s, mc));
#endif
		DBGTRACE(TC_GC_TRACING,
		         "gc (", fmt_u64d(buf, s->iter_count), ") ",
		         "iter free_pending: ", fmt_ptr(buf2, mc), " ");
		DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });
		assert(!memcell_locked(mc)); // locked nodes must stay in root list
		/* TODO: what about loops here? should they still be unlinked? */
		assert(!memcell_refcount(mc)); // free_pending nodees must be unlinked
		s->dl_cb(dl_cb_decref_free_pending_z, &(mc->data), s);
		memcell_free(s, mc);
#if ! defined(NO_GC_FREELIST)
		DBGTRACELN(TC_MEM_ALLOC,
		           "gc (", fmt_u64d(buf, s->iter_count), "): ",
		           "free node (pending) ", fmt_ptr(buf2, mc), " ",
		           "nfree=", fmt_u64d(buf3, s->total_free));
		assert(s->total_free <= s->total_alloc);
#endif /* ! defined(NO_GC_FREELIST) */
		goto finish;
	}

	/* process boundary nodes: move them to reachable */
	if(! dlist_is_empty(&(s->boundary_list))) {
		mc = (memcell_t *) dlist_first(&(s->boundary_list));
		DBGTRACE(TC_GC_TRACING,
		         "gc (", fmt_u64d(buf, s->iter_count), ") ",
		         "iter reachable: ", fmt_ptr(buf2, mc), " ");
		DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });
		assert(!memcell_locked(mc)); // locked nodes should stay in root list
		assert(memcell_refcount(mc)); // referenced nodes should have refcount
		s->dl_cb(dl_cb_try_move_boundary, &(mc->data), s);
		dlnode_remove(&(mc->hdr));
		memcell_to_reachable(s, mc);
		goto finish;
	}

	/* check to see if there are any root nodes to process: move their links
	   to boundary */
	if(dlist_first(&(s->roots_list)) != &(s->root_sentinel)) {
		mc = (memcell_t *) dlist_first(&(s->roots_list));
		DBGTRACE(TC_GC_TRACING,
		         "gc (", fmt_u64d(buf, s->iter_count), ") ",
		         "iter root: ", fmt_ptr(buf2, mc), " ");
		DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });
		s->dl_cb(dl_cb_try_move_boundary, &(mc->data), s);
		/* after processing, rotate to end of list */
		dlnode_remove(&(mc->hdr));
		memcell_to_roots_proc(s, mc);
		goto finish;
	}

	/* remaining 'unprocessed' nodes are unreachable: 'free' them */
	if(! dlist_is_empty(s->unproc_listref)) {
		mc = (memcell_t *) dlist_first(s->unproc_listref);
#if defined(GC_REACHABILITY_VERIFICATION)
		assert(!memcell_reachable(s, mc));
#endif
		DBGTRACE(TC_GC_TRACING,
		         "gc (", fmt_u64d(buf, s->iter_count), ") ",
		         "iter reachable: ", fmt_ptr(buf2, mc), " ");
		DBGRUN(TC_GC_TRACING, { s->p_cb(mc->data, dbgtrace_getstream()); });
		assert(! memcell_locked(mc)); // locked nodes should stay in root list
		// NB: unreachable can be referenced if e.g. lambda points back to it.
		s->dl_cb(dl_cb_decref_free_pending_z, &(mc->data), s);
		memcell_free(s, mc);
#if defined(NO_GC_FREELIST)
		DBGTRACELN(TC_MEM_ALLOC,
		         "gc (", fmt_u64d(buf, s->iter_count), "): ",
		         "free node (unreachable) ", fmt_ptr(buf2, mc));
#else /* ! defined(NO_GC_FREELIST) */
		DBGTRACELN(TC_MEM_ALLOC,
		         "gc (", fmt_u64d(buf, s->iter_count), "): ",
		         "free node (unreachable) ", fmt_ptr(buf2, mc), " ",
		         "nfree=", fmt_u64d(buf3, s->total_free));
		assert(s->total_free <= s->total_alloc);
#endif /* ? defined(NO_GC_FREELIST) */
		goto finish;
	}

	/* reset state -- progress root_sentinel, swap reachable/unproc lists */
	DBGTRACELN(TC_GC_TRACING, "gc iter ** done **: reset state");
	assert(dlist_first(&(s->roots_list)) == &(s->root_sentinel));
	dlist_insertlast(&(s->roots_list),
	                 dlnode_remove(dlist_first(&(s->roots_list))));
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
	s->clean_cycles++;

finish:
	DBGRUN(TC_GC_VERBOSE, {  memory_gc_print_state(s, dbgtrace_getstream()); });
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

	if(mc->mc_flags.searched) {
		return;
	}

	mc->mc_flags.searched = true;

	if(mc == ri->dest) {
		ri->found = true;
	} else {
		ri->s->dl_cb(reachable_helper, &(mc->data), ri);
	}

	mc->mc_flags.searched = false;
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

static void memcell_print_meta(
	memory_state_t *s,
	memcell_t *mc,
	stream_t *stream)
{
	char buf[21], buf2[21];
	char *listname = NULL;
	unsigned long long refcount;
	bool reachable, locked;

	if(memcell_is_free(s, mc)) listname = "free";
	else if(mc->hdr.owner == &(s->roots_list)) listname = "root";
	else if(mc->hdr.owner == &(s->boundary_list) )listname = "boundary";
	else if(mc->hdr.owner == &(s->free_pending_list) )listname = "free_pend";
	else if(mc->hdr.owner == s->reachable_listref) listname = "reachable";
	else if(mc->hdr.owner == s->unproc_listref) listname = "unproc";
	else assert(false);

	refcount = memcell_refcount(mc);
	reachable = memcell_reachable(s, mc);
	locked = memcell_locked(mc);

	stream_put(stream,
	          listname, " ",
	          fmt_ptr(buf, mc), " ",
	          "refcount=", fmt_u64d(buf2, refcount), " ",
	          "reachable=", (reachable ? "y" : "n"),
	          "locked=", (locked ? "y": "n"));
}

void memory_gc_print_state(memory_state_t *state, stream_t *stream)
{
	dlnode_t *cursor;
	memcell_t *mc;
	char buf[21], buf2[21], buf3[21];

	stream_putln(stream,
	             "gc (", fmt_s64(buf, state->iter_count), ") ",
	             "state ", fmt_ptr(buf2, state), " ",
	             "(", fmt_s64(buf3, state->total_alloc), "):");

	DLIST_FOR_FWD(&(state->free_pending_list), cursor) {
		mc = (memcell_t *) cursor;
		memcell_print_meta(state, mc, stream);
		state->p_cb(mc->data, stream);
	}

	DLIST_FOR_FWD(&(state->roots_list), cursor) {
		if(cursor == &(state->root_sentinel)) {
			stream_putln(stream, "root sentinel ", fmt_ptr(buf, cursor));
		} else {
			mc = (memcell_t *) cursor;
			memcell_print_meta(state, mc, stream);
			state->p_cb(mc->data, stream);
		}
	}

	DLIST_FOR_FWD(&(state->boundary_list), cursor) {
		mc = (memcell_t *) cursor;
		memcell_print_meta(state, mc, stream);
		state->p_cb(mc->data, stream);
	}

	DLIST_FOR_FWD(state->reachable_listref, cursor) {
		mc = (memcell_t *) cursor;
		memcell_print_meta(state, mc, stream);
		state->p_cb(mc->data, stream);
	}

	DLIST_FOR_FWD(state->unproc_listref, cursor) {
		mc = (memcell_t *) cursor;
		memcell_print_meta(state, mc, stream);
		state->p_cb(mc->data, stream);
	}

#if ! defined(NO_GC_FREELIST)
	DLIST_FOR_FWD(&(state->free_list), cursor) {
		mc = (memcell_t *) cursor;
		memcell_print_meta(state, mc, stream);
		state->p_cb(mc->data, stream);
	}
#endif
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
#if ! defined(NO_GC_FREELIST)
	return s->total_free;
#else
	return 0;
#endif
}
