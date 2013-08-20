#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "frame.h"

void frame_push(
	node_t *frame_hdl,
	node_t **locals,
	node_t **newvals,
	size_t n)
{
	ssize_t i;
	node_t *cursor = NULL;

	for(i = n - 1; i >= 0; i--) {
		cursor = node_cons_new(node_handle(locals[i]), cursor);
		node_handle_update(locals[i], newvals[i]);
	}
	cursor = node_cons_new(cursor, node_handle(frame_hdl));
	node_handle_update(frame_hdl, cursor);
}

void frame_pop(
	node_t *frame_hdl,
	node_t **locals,
	size_t n)
{
	size_t i;
	node_t *cursor;

	cursor = node_cons_car(node_handle(frame_hdl));
	for(i = 0; i < n; i++) {
		assert(cursor);
		node_handle_update(locals[i], node_cons_car(cursor));
		cursor = node_cons_cdr(cursor);
	}

	node_handle_update(frame_hdl, node_cons_cdr(node_handle(frame_hdl)));
}

void frame_restore(
	node_t *frame_hdl,
	node_t **locals,
	size_t n,
	node_t *snapshot)
{
	node_handle_update(frame_hdl, snapshot);
	frame_pop(frame_hdl, locals, n);
}

void frame_add_elem(
	node_t *frame_hdl,
	node_t *val)
{
	node_t *oldframe = node_handle(frame_hdl);

	node_t *nextent = node_cons_car(oldframe);
	node_t *nextfrm = node_cons_cdr(oldframe);

	node_t *newent = node_cons_new(val, nextent);
	node_t *newframe = node_cons_new(newent, nextfrm);

	node_handle_update(frame_hdl, newframe);
}

int frame_iterate(
	node_t *frame_hdl,
	frm_cb_t e_cb, void *e_p,
	bool recursive,
	frm_cb_t f_cb, void *f_p)
{
	node_t *frame_curs, *var_curs, *val;
	int status;

	frame_curs = node_handle(frame_hdl);
restart:
	if(!frame_curs) {
		return 0;
	}

	assert(node_type(frame_curs) == NODE_CONS);
	if(f_cb) {
		if((status = f_cb(frame_curs, f_p))) {
			return status;
		}
	}

	if(e_cb) {
		for(var_curs = node_cons_car(frame_curs);
		    var_curs;
		    var_curs = node_cons_cdr(var_curs)) {

			assert(node_type(var_curs) == NODE_CONS);
			val = node_cons_car(var_curs);
			if((status = e_cb(val, e_p))) {
				 return status;
			}
		}
	}

	frame_curs = node_cons_cdr(frame_curs);
	if(recursive) {
		goto restart;
	}
	return 0;
}
	
void frame_init(
	node_t **locals,
	size_t n)
{
	size_t i;
	for(i = 0; i < n; i++) {
		locals[i] = node_lockroot(node_handle_new(NULL));
	}
}

void frame_deinit(
	node_t **locals,
	size_t n)
{
	size_t i;
	for(i = 0; i < n; i++) {
		node_droproot(locals[i]);
	}
}

static int print_frm_cb(node_t *n, void *p)
{
	char *fmt = (char *) p;
	printf(fmt, n);
	node_print(n);
	return 0;
}

void frame_print_bt(node_t *frame_hdl)
{
	frame_iterate(frame_hdl,
	              print_frm_cb, "frame @ %p: ",
	              true,
	              print_frm_cb, "    val @ %p: ");
}