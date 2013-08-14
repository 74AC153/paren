#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "frame.h"

/* frame format:

   frame_hdl
   +---+
   |   |
   +---+
     |
     |
     v
   top frame    (parent frame)
   +---+---+    +---+---+
   |   | ------>|   | -----> ...
   +-|-+---+    +-|-+---+
     |            v
     |            ...
     |
     v
   entry        (next entry)
   +---+---+    +---+---+
   |   | ------>|   | -----> ...
   +-|-+---+    +-|-+---+
     |            |
     v            v
   val_hdl
   +---+
   |   |
   +-|-+
     |
     v
    val 
   +---+
   | ? |
   +---+
*/

void frame_push(
	node_t *frame_hdl,
	struct eval_frame *locals,
	node_t *in,
	node_t *env_handle)
{
	node_t *newframe, *oldframe;

	assert(node_type(frame_hdl) == NODE_HANDLE);
	assert(locals);
	assert(node_type(env_handle) == NODE_HANDLE);

	oldframe = node_handle(frame_hdl);
	newframe = NULL;

	newframe = node_cons_new(node_handle(locals->state_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->restart_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->cursor_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->newargs_last_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->newargs_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->env_hdl_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->func_hdl), newframe);
	newframe = node_cons_new(node_handle(locals->in_hdl), newframe);

	newframe = node_cons_new(newframe, oldframe);
	node_handle_update(frame_hdl, newframe);

	node_handle_update(locals->state_hdl, NULL);
	node_handle_update(locals->restart_hdl, NULL);
	node_handle_update(locals->cursor_hdl, NULL);
	node_handle_update(locals->newargs_last_hdl, NULL);
	node_handle_update(locals->newargs_hdl, NULL);
	node_handle_update(locals->env_hdl_hdl, env_handle);
	node_handle_update(locals->func_hdl, NULL);
	node_handle_update(locals->in_hdl, in);
}

void frame_pop(
	node_t *frame_hdl,
	struct eval_frame *locals)
{
	node_t *newframe, *cursor;

	assert(node_type(frame_hdl) == NODE_HANDLE);
	assert(node_handle(frame_hdl));
	assert(locals);

	newframe = node_cons_cdr(node_handle(frame_hdl));
	cursor = node_cons_car(node_handle(frame_hdl));
	assert(cursor);

	node_handle_update(locals->in_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->func_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->env_hdl_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->newargs_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->newargs_last_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->cursor_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->restart_hdl, node_cons_car(cursor));
	cursor = node_cons_cdr(cursor);
	assert(cursor);
	node_handle_update(locals->state_hdl, node_cons_car(cursor));

	node_handle_update(frame_hdl, newframe);
}

void frame_restore(
	node_t *frame_hdl,
	struct eval_frame *locals,
	node_t *snapshot)
{
	/* push an empty dummy frame, then reuse frame_pop */	
	node_t *dummy = node_cons_new(NULL, snapshot);
	node_handle_update(frame_hdl, dummy);
	frame_pop(frame_hdl, locals);
}

void frame_print_bt(node_t *frame_hdl)
{
	node_t *frame_curs, *var_curs, *val;

	for(frame_curs = node_handle(frame_hdl);
	    frame_curs;
	    frame_curs = node_cons_cdr(frame_curs)) {

		assert(node_type(frame_curs) == NODE_CONS);
		printf("frame @ %p\n", frame_curs);

		for(var_curs = node_cons_car(frame_curs);
		    var_curs;
		    var_curs = node_cons_cdr(var_curs)) {

			assert(node_type(var_curs) == NODE_CONS);
			val = node_cons_car(var_curs);
			printf("val @ %p ", val);
			node_print(val);
		}
		printf("\n");
	}
}

void framearr_push(
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

void framearr_pop(
	node_t *frame_hdl,
	node_t **locals,
	size_t n)
{
	size_t i;
	node_t *cursor;

	node_handle_update(frame_hdl, node_cons_cdr(node_handle(frame_hdl)));

	cursor = node_cons_car(node_handle(frame_hdl));
	for(i = 0; i < n; i++) {
		assert(cursor);
		node_handle_update(locals[i], node_cons_car(cursor));
		cursor = node_cons_cdr(cursor);
	}
	assert(!cursor);
}

void framearr_restore(
	node_t *frame_hdl,
	node_t **locals,
	size_t n,
	node_t *snapshot)
{
	/* push an empty dummy frame, then reuse frame_pop */	
	node_t *dummy = node_cons_new(NULL, snapshot);
	node_handle_update(frame_hdl, dummy);
	framearr_pop(frame_hdl, locals, n);
}
	
void framearr_init(
	node_t **locals,
	size_t n)
{
	size_t i;
	for(i = 0; i < n; i++) {
		locals[i] = node_lockroot(node_handle_new(NULL));
	}
}

void framearr_deinit(
	node_t **locals,
	size_t n)
{
	size_t i;
	for(i = 0; i < n; i++) {
		node_droproot(locals[i]);
	}
}
