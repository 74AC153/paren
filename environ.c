#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "node.h"
#include "environ.h"

#if 0
void environ_pushframe(node_t **environ)
{
	node_t *oldenv = *environ;
	assert(! *environ || node_isroot(*environ));
	*environ = node_cons_new(NULL, *environ);
	node_lockroot(*environ);
	node_droproot(oldenv);
	assert(*environ && node_isroot(*environ));
}

void environ_popframe(node_t **environ)
{
	node_t *oldenv = *environ;
	assert(! *environ || node_isroot(*environ));
	*environ = node_cons_cdr(*environ);
	node_lockroot(*environ);
	node_droproot(oldenv);
	assert(! *environ || node_isroot(*environ));
}

void environ_add(node_t **environ, node_t *key, node_t *val)
{
	assert(! *environ || node_isroot(*environ));

	node_t *nextent = node_cons_car(*environ);
	node_t *nextfrm = node_cons_cdr(*environ);

	node_t *newkv = node_cons_new(key, val);
	node_t *newent = node_cons_new(newkv, nextent);
	node_t *newfrm = node_cons_new(newent, nextfrm);
	node_lockroot(newfrm);
	node_droproot(*environ);
	*environ = newfrm;
	assert(*environ && node_isroot(*environ));
}
#endif

void environ_pushframe(node_t *env_handle)
{
	node_t *oldenv, *newenv;
	assert(env_handle);
	oldenv = node_handle(env_handle);
	newenv = node_cons_new(NULL, oldenv);
	node_handle_update(env_handle, newenv);
}

void environ_popframe(node_t *env_handle)
{
	node_t *oldenv, *newenv;
	assert(env_handle);
	oldenv = node_handle(env_handle);
	newenv = node_cons_cdr(oldenv);
	node_handle_update(env_handle, newenv);
}

void environ_add(node_t *env_handle, node_t *key, node_t *val)
{
	node_t *oldenv = node_handle(env_handle);

	node_t *nextent = node_cons_car(oldenv);
	node_t *nextfrm = node_cons_cdr(oldenv);

	node_t *newkv = node_cons_new(key, val);
	node_t *newent = node_cons_new(newkv, nextent);
	node_t *newenv = node_cons_new(newent, nextfrm);

	node_handle_update(env_handle, newenv);
}

bool environ_keyval_frame(node_t *top_frame, node_t *key, node_t **keyval)
{
	node_t *entry_curs, *kv, *testkey;

	for(entry_curs = node_cons_car(top_frame);
	    entry_curs;
	    entry_curs = node_cons_cdr(entry_curs)) {

		kv = node_cons_car(entry_curs);
		testkey = node_cons_car(kv);

		if(!strcmp(node_symbol_name(testkey), node_symbol_name(key))) {
			*keyval = kv;
			return true;
		}
		
	}
	return false;
}

bool environ_keyval(node_t *top_frame, node_t *key, node_t **keyval)
{
	node_t *frame_curs;

	for (frame_curs = top_frame;
	     frame_curs;
	     frame_curs = node_cons_cdr(frame_curs)) {

		if(environ_keyval_frame(frame_curs, key, keyval)) {
			return true;
		}
	}
	return false;
}

bool environ_lookup(node_t *top_frame, node_t *key, node_t **value)
{
	node_t *keyval;

	if(! environ_keyval(top_frame, key, &keyval)) {
		return false;
	}
	*value = node_cons_cdr(keyval);
	return true;
}

void environ_print(node_t *top_frame)
{
	node_t *frame_curs, *entry_curs, *keyval;

	printf("environ %p:\n", top_frame);
	for (frame_curs = top_frame;
	     frame_curs;
	     frame_curs = node_cons_cdr(frame_curs)) {

		for (entry_curs = node_cons_car(frame_curs);
		     entry_curs;
		     entry_curs = node_cons_cdr(entry_curs)) {

			keyval = node_cons_car(entry_curs);
			node_print_pretty(node_cons_car(keyval), false);
			printf(" -> ");
			node_print_pretty(node_cons_cdr(keyval), false);
			printf("\n");

		}
		printf("\n");
	}
	printf("\n");
}

