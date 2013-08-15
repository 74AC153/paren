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
	assert(!cursor);

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

