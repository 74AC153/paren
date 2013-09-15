#if ! defined(BUFSTREAM_H)
#define BUFSTREAM_H

#include <stddef.h>
#include "stream.h"

typedef struct {
	stream_t hdr;
	unsigned char *buf;
	size_t len;
	size_t roff;
} bufstream_t;

stream_t *bufstream_init(void *p, unsigned char *start, size_t len);

#endif
