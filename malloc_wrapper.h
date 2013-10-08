#if ! defined(LIBC_MALLOC_WRAP_H)
#define LIBC_MALLOC_WRAP_H

#include <stddef.h>

void *libc_malloc_wrap(size_t len, void *priv);
void libc_free_wrap(void *mem, void *priv);

#endif
