#include <stdlib.h>

void *libc_malloc_wrap(size_t len, void *priv)
{
	return malloc(len);
}

void libc_free_wrap(void *mem, void *priv)
{
	free(mem);
}

