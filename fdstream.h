#if ! defined(FDSTREAM_H)
#define FDSTREAM_H

#include <unistd.h>
#include "stream.hf"

typedef struct {
	stream_t hdr;
	int fd;
} fdstream_t;

stream_t *fdstream_init(void *p, int fd);

#endif
