#if ! defined(BUFSTREAM_H)
#define BUFSTREAM_H

#include <stddef.h>

typedef struct {
	unsigned char *cursor;
	size_t len;
} bufstream_t;

int bufstream_readch(void *p);
bufstream_t *bufstream_init(void *p, unsigned char *start, size_t len);

#endif
