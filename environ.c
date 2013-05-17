#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "node.h"
#include "environ.h"

void environ_pushframe(node_t **environ)
{
	node_t *oldenv = *environ;
	assert(! *environ || node_is_remembered(*environ));
	*environ = node_new_list(NULL, *environ);
	node_remember(*environ);
	node_forget(oldenv);
	assert(*environ && node_is_remembered(*environ));
}

void environ_popframe(node_t **environ)
{
	node_t *oldenv = *environ;
	assert(! *environ || node_is_remembered(*environ));
	*environ = node_next_noref(*environ);
	node_remember(*environ);
	node_forget(oldenv);
	assert(! *environ || node_is_remembered(*environ));
}

void environ_add(node_t **environ, node_t *key, node_t *val)
{
	assert(! *environ || node_is_remembered(*environ));

	node_t *nextent = node_child_noref(*environ);
	node_t *nextfrm = node_next_noref(*environ);

	node_t *newkv = node_new_list(key, val);
	node_t *newent = node_new_list(newkv, nextent);
	node_t *newfrm = node_new_list(newent, nextfrm);
	node_remember(newfrm);
	node_forget(*environ);
	*environ = newfrm;
	assert(*environ && node_is_remembered(*environ));
}

bool environ_keyvalue_frame(node_t *frame, node_t *key, node_t **keyvalue)
{
	node_t *entry_curs, *kv, *testkey;

	for(entry_curs = node_child_noref(frame);
	    entry_curs;
	    entry_curs = node_next_noref(entry_curs)) {

		kv = node_child_noref(entry_curs);
		testkey = node_child_noref(kv);

		if(!strcmp(node_name(testkey), node_name(key))) {
			*keyvalue = kv;
			return true;
		}
		
	}
	return false;
}

bool environ_keyvalue(node_t *environ, node_t *key, node_t **keyvalue)
{
	node_t *frame_curs;

	for (frame_curs = environ;
	     frame_curs;
	     frame_curs = node_next_noref(frame_curs)) {

		if(environ_keyvalue_frame(frame_curs, key, keyvalue)) {
			return true;
		}
	}
	return false;
}

bool environ_lookup(node_t *environ, node_t *key, node_t **value)
{
	node_t *keyvalue;

	if(! environ_keyvalue(environ, key, &keyvalue)) {
		return false;
	}
	*value = node_next_noref(keyvalue);
	return true;
}

void environ_print(node_t *environ)
{
	node_t *frame_curs, *entry_curs, *keyval;

	printf("environ %p:\n", environ);
	for (frame_curs = environ;
	     frame_curs;
	     frame_curs = node_next_noref(frame_curs)) {

		for (entry_curs = node_child_noref(frame_curs);
		     entry_curs;
		     entry_curs = node_next_noref(entry_curs)) {

			keyval = node_child_noref(entry_curs);
			node_print_pretty(node_child_noref(keyval));
			printf(" -> ");
			node_print_pretty(node_next_noref(keyval));
			printf("\n");

		}
		printf("\n");
	}
	printf("\n");
}
