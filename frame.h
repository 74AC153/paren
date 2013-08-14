#if ! defined(FRAME_H)
#define FRAME_H

#include "node.h"

struct eval_frame {
	node_t *state_hdl;
	node_t *restart_hdl;
	node_t *cursor_hdl;
	node_t *newargs_last_hdl;
	node_t *newargs_hdl;
	node_t *env_hdl_hdl;
	node_t *func_hdl;
	node_t *in_hdl;
};

void frame_push(
	node_t *frame_hdl,
	struct eval_frame *locals,
	node_t *in,
	node_t *env_handle);

void frame_pop(
	node_t *frame_hdl,
	struct eval_frame *locals);

void frame_restore(
	node_t *frame_hdl,
	struct eval_frame *locals,
	node_t *snapshot);

void frame_print_bt(node_t *frame_hdl);

void framearr_push(
	node_t *frame_hdl,
	node_t **locals,
	node_t **newvals,
	size_t n);

void framearr_pop(
	node_t *frame_hdl,
	node_t **locals,
	size_t n);

void framearr_restore(
	node_t *frame_hdl,
	node_t **locals,
	size_t n,
	node_t *snapshot);
	
void framearr_init(
	node_t **locals,
	size_t n);

void framearr_deinit(
	node_t **locals,
	size_t n);

#endif
