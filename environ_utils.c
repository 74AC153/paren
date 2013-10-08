#include "environ_utils.h"
#include "memory.h"

void environ_add_builtins(node_t *env_handle,
                          foreign_assoc_t *builtins,
                          size_t num)
{
	size_t i;
	node_t *key, *value;
	memory_state_t *s = data_to_memstate(env_handle);

	for (i = 0; i < num; i++) {
		key = node_symbol_new(s, builtins[i].name);
		value = builtins[i].func(s);

		environ_add(env_handle, key, value);
	}
}
