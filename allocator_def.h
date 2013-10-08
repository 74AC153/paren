#if ! defined(ALLOCATOR_DEF_H)
#define ALLOCATOR_DEF_H

#include <stddef.h>

typedef void * (*mem_allocator_fn_t)(size_t len, void *p);
typedef void (*mem_free_fn_t)(void *p);

#endif
