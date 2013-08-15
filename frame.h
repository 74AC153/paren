#if ! defined(FRAME_H)
#define FRAME_H

#include "node.h"

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
	
void frame_init(
	node_t **locals,
	size_t n);

void frame_deinit(
	node_t **locals,
	size_t n);

void frame_print_bt(node_t *frame_hdl);

#endif
