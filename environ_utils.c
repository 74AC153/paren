#include "environ_utils.h"

void environ_add_builtins(node_t **env,
                          foreign_assoc_t *builtins,
                          size_t num)
{
	size_t i;
	node_t *key, *value;

	for (i = 0; i < num; i++) {
		key = node_symbol_new(builtins[i].name);
		value = builtins[i].func();

		environ_add(env, key, value);
	}
}
