#if ! defined(FRAME_H)
#define FRAME_H

#include "node.h"

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
     v
   entry        (next entry)
   +---+---+    +---+---+
   |   | ------>|   | -----> ...
   +-|-+---+    +-|-+---+
     |            v
     v            ...
    val 
   +---+
   | ? |
   +---+
*/

void frame_push(
	node_t *frame_hdl,
	node_t **locals,
	node_t **newvals,
	size_t n);

void frame_pop(
	node_t *frame_hdl,
	node_t **locals,
	size_t n);

void frame_restore(
	node_t *frame_hdl,
	node_t **locals,
	size_t n,
	node_t *snapshot);

void frame_add_elem(
	node_t *frame_hdl,
	node_t *val);

typedef int (*frm_cb_t)(node_t *n, void *p);
int frame_iterate(
	node_t *frame_hdl,
	frm_cb_t e_cb, void *e_p,
	bool recursive,
	frm_cb_t f_cb, void *f_p);
	
void frame_init(
	memory_state_t *ms,
	node_t **locals,
	size_t n);

void frame_deinit(
	node_t **locals,
	size_t n);

void frame_print_bt(
	node_t *frame_hdl,
	stream_t *stream);

#endif
