#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"
#include "environ.h"

node_t *environ_push(node_t *environ)
{
	return node_new_list(NULL, environ);
}

node_t *environ_parent(node_t *environ)
{
	return node_next(environ);
}

node_t *environ_add(node_t *environ, node_t *key, node_t *val)
{
	node_t *keyval = node_new_list(key, val);
	node_t *oldchild = node_child(environ);
	node_t *newchild = node_new_list(keyval, oldchild);
	node_t *envnext = node_next(environ);
	node_t *newenv = node_new_list(newchild, envnext);

	node_release(keyval);
	node_release(oldchild);
	node_release(newchild);
	node_release(envnext);

	return newenv;
}

bool environ_lookup(node_t *environ, node_t *key, node_t **value)
{
	node_t *env_curs, *child_curs, *keyval, *testkey, *val;

	for(env_curs = environ;
	    env_curs;
	    env_curs = node_next_noref(env_curs)) {
		for(child_curs = node_child_noref(env_curs);
		    child_curs; 
		    child_curs = node_next_noref(child_curs)) {
			keyval = node_child_noref(child_curs);
			testkey = node_child_noref(keyval);
			val = node_next_noref(keyval);

			if(!strcmp(node_name(testkey), node_name(key))) {
				*value = node_retain(val);
				return true;
			}
			
		}
	}
	return false;
}

void environ_print(node_t *environ)
{
	node_t *env_curs, *child_curs, *keyval;

	for(env_curs = environ; env_curs; env_curs = node_next_noref(env_curs)) {
		for(child_curs = node_child_noref(env_curs);
		    child_curs;
		    child_curs = node_next_noref(child_curs)) {
			keyval = node_child_noref(child_curs);
			printf("key: ");
			node_print_pretty(node_child_noref(keyval));
			printf("\nvalue: ");
			node_print_pretty(node_next_noref(keyval));
			printf("\n");
		}
		printf("\n");
	}
}
