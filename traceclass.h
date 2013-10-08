#if ! defined(TRACECLASS_H)
#define TRACECLASS_H

#include <stdbool.h>

#define TRACECLASSES \
X_GROUP(TC_NONE,       0x0000000000000000) \
X_ITEM (TC_MEM_ALLOC,  0x0000000000000001) \
X_ITEM (TC_GC_TRACING, 0x0000000000000002) \
X_ITEM (TC_MEM_RC,     0x0000000000000004) \
X_ITEM (TC_GC_VERBOSE, 0x0000000000000008) \
X_ITEM (TC_NODE_GC,    0x0000000000000010) \
X_ITEM (TC_NODE_INIT,  0x0000000000000020) \
X_ITEM (TC_EVAL,       0x0000000000000040) \
X_ITEM (TC_FMC_ALLOC,  0x0000000000000080) \
X_GROUP(TC_ALL,        0xFFFFFFFFFFFFFFFF) \


#define X_GROUP(C, V) static const uint64_t C = V;
#define X_ITEM(C, V) static const uint64_t C = V;
TRACECLASSES
#undef X_ITEM
#undef X_GROUP

typedef uint64_t traceclass_t;

typedef uint64_t tracemask_t;

static inline void tracemask_set(tracemask_t *m, traceclass_t c)
{
	*m |= c;
}

static inline void tracemask_reset(tracemask_t *m, traceclass_t c)
{
	*m &= ~c;
}

static inline bool tracemask_test(tracemask_t *m, traceclass_t c)
{
	return *m & c;
}

#endif
