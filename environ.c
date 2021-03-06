#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "node.h"
#include "environ.h"
#include "frame.h"
#include "stream.h"
#include "libc_custom.h"

void environ_pushframe(node_t *env_handle)
{
	frame_push(env_handle, NULL, NULL, 0);
}

void environ_popframe(node_t *env_handle)
{
	frame_pop(env_handle, NULL, 0);
}

void environ_add(node_t *env_handle, node_t *key, node_t *val)
{
	memory_state_t *ms = data_to_memstate(env_handle);
	frame_add_elem(env_handle, node_cons_new(ms, key, val));
}

struct kv_find
{
	node_t *key;
	node_t *keyval;
};

static int find_keyval(node_t *kv, void *p)
{
	struct kv_find *info = (struct kv_find *) p;
	node_t *testkey;

	testkey = node_cons_car(kv);

	if(!strcmp(node_symbol_name(testkey), node_symbol_name(info->key))) {
		info->keyval = kv;
		return 1;
	}
	return 0;
}

static bool environ_find(
	node_t *env_handle,
	node_t *key,
	bool recursive,
	node_t **keyval)
{
	struct kv_find info = { .key = key, .keyval = NULL };
	int status;
	status = frame_iterate(env_handle, find_keyval, &info, recursive,
	                       NULL, NULL);
	if(keyval) {
		*keyval = info.keyval;
	}
	return status != 0;
}

bool environ_keyval_frame(node_t *env_handle, node_t *key, node_t **keyval)
{
	return environ_find(env_handle, key, false, keyval);
}

bool environ_keyval(node_t *env_handle, node_t *key, node_t **keyval)
{
	return environ_find(env_handle, key, true, keyval);
}

bool environ_lookup(node_t *env_handle, node_t *key, node_t **val)
{
	int status;
	node_t *keyval;
	status = environ_find(env_handle, key, true, &keyval);
	if(val) {
		*val = node_cons_cdr(keyval);
	}
	return status != 0;
}

static int print_env_cb(node_t *env, void *p)
{
	char buf[17];
	stream_t *s = (stream_t *) p;
	stream_putln(s, "env frame @ ", fmt_ptr(buf, env), ":");
	return 0;
}

static int print_val_cb(node_t *kv, void *p)
{
	stream_t *s = (stream_t *) p;
	stream_putstr(s, "    ");
	node_print_pretty_stream(s, node_cons_car(kv), false);
	stream_putstr(s, " -> ");
	node_print_pretty_stream(s, node_cons_cdr(kv), false);
	stream_putch(s, '\n');
	return 0;
}

void environ_print(node_t *env_handle, stream_t *stream)
{
	frame_iterate(env_handle,
	              print_val_cb, stream, true,
	              print_env_cb, stream);
}
