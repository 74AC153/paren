#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "node.h"
#include "environ.h"

void environ_pushframe(node_t **environ)
{
	node_t *oldenv = *environ;
	assert(! *environ || node_isroot(*environ));
	*environ = node_cons_new(NULL, *environ);
	node_makeroot(*environ);
	node_droproot(oldenv);
	assert(*environ && node_isroot(*environ));
}

void environ_popframe(node_t **environ)
{
	node_t *oldenv = *environ;
	assert(! *environ || node_isroot(*environ));
	*environ = node_cons_cdr(*environ);
	node_makeroot(*environ);
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
	node_makeroot(newfrm);
	node_droproot(*environ);
	*environ = newfrm;
	assert(*environ && node_isroot(*environ));
}

bool environ_keyvalue_frame(node_t *frame, node_t *key, node_t **keyvalue)
{
	node_t *entry_curs, *kv, *testkey;

	for(entry_curs = node_cons_car(frame);
	    entry_curs;
	    entry_curs = node_cons_cdr(entry_curs)) {

		kv = node_cons_car(entry_curs);
		testkey = node_cons_car(kv);

		if(!strcmp(node_symbol_name(testkey), node_symbol_name(key))) {
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
	     frame_curs = node_cons_cdr(frame_curs)) {

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
	*value = node_cons_cdr(keyvalue);
	return true;
}

void environ_print(node_t *environ)
{
	node_t *frame_curs, *entry_curs, *keyval;

	printf("environ %p:\n", environ);
	for (frame_curs = environ;
	     frame_curs;
	     frame_curs = node_cons_cdr(frame_curs)) {

		for (entry_curs = node_cons_car(frame_curs);
		     entry_curs;
		     entry_curs = node_cons_cdr(entry_curs)) {

			keyval = node_cons_car(entry_curs);
			node_print_pretty(node_cons_car(keyval));
			printf(" -> ");
			node_print_pretty(node_cons_cdr(keyval));
			printf("\n");

		}
		printf("\n");
	}
	printf("\n");
}
