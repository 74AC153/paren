#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"
#include "environ.h"
void environ_add(node_t **environ, node_t *key, node_t *val)
{
	node_t *newenv;
	
	newenv = node_retain(node_new_list(node_new_list(key, val), *environ));
	node_release(*environ);

	*environ = newenv;
}

bool environ_keyvalue(node_t *environ, node_t *key, node_t **keyvalue)
{
	node_t *env_curs, *kv, *testkey;

	for(env_curs = environ; env_curs; env_curs = node_next_noref(env_curs)) {
		kv = node_child_noref(env_curs);
		testkey = node_child_noref(kv);

		if(!strcmp(node_name(testkey), node_name(key))) {
			*keyvalue = node_retain(kv);
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

void environ_print(node_t *environ)
{
	node_t *env_curs, *keyval;

	for(env_curs = environ;
	    env_curs;
	    env_curs = node_next_noref(env_curs)) {
		keyval = node_child_noref(env_curs);
		node_print_pretty(node_child_noref(keyval));
		printf(" -> ");
		node_print_pretty(node_next_noref(keyval));
		printf("\n");
	}
	printf("\n");
}
