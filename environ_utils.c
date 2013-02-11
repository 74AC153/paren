#include "environ_utils.h"

node_t *environ_add_builtins(node_t*env,
                             builtin_assoc_t *builtins,
                             size_t num)
{
	size_t i;
	node_t *key, *value, *oldenv, *newenv;

	newenv = env;
	for (i = 0; i < num; i++) {
		key = node_new_symbol(builtins[i].name);
		value = node_new_builtin(builtins[i].func);
		oldenv = newenv;

		newenv = environ_add(oldenv, key, value);

		node_release(oldenv);
		node_release(value);
		node_release(key);
	}

	return newenv;
}
