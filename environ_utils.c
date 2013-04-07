#include "environ_utils.h"

node_t *environ_add_builtins(node_t*env,
                             builtin_assoc_t *builtins,
                             size_t num)
{
	size_t i;
	node_t *key, *value;

	for (i = 0; i < num; i++) {
		key = node_new_symbol(builtins[i].name);
		value = node_new_builtin(builtins[i].func);

		environ_add(&env, key, value);
	}

	return env;
}
