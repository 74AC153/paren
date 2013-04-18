#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"
#include "environ.h"

void environ_pushframe(node_t **environ)
{
	node_t *oldenv = *environ;
	*environ = node_retain(node_new_list(NULL, *environ));
	node_release(oldenv);
}

void environ_popframe(node_t **environ)
{
	node_t *oldenv = *environ;
	*environ = node_retain(node_next_noref(*environ));
	node_release(oldenv);
}

void environ_add(node_t **environ, node_t *key, node_t *val)
{
	node_t *nextent = node_child_noref(*environ);
	node_t *nextfrm = node_next_noref(*environ);

	node_t *newkv = node_new_list(key, val);
	node_t *newent = node_new_list(newkv, nextent);
	node_t *newfrm = node_retain(node_new_list(newent, nextfrm));
	
	node_release(*environ);
	*environ = newfrm;
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
			*keyvalue = node_retain(kv);
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
	*value = node_retain(node_next_noref(keyvalue));
	node_release(keyvalue);
	return true;
}

#if 0
bool environ_lookup_frame(node_t *environ, node_t *key, node_t **value)
{
	node_t *keyvalue;

	if(! environ_keyvalue_frame(environ, key, &keyvalue)) {
		return false;
	}
	*value = node_retain(node_next_noref(keyvalue));
	node_release(keyvalue);
	return true;
}
#endif

void environ_print(node_t *environ)
{
	node_t *frame_curs, *entry_curs, *keyval;

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
