#if ! defined(FDSTREAM_H)
#define FDSTREAM_H

#include "stream.h"

typedef struct {
	stream_t hdr;
	int fd;
} fdstream_t;

extern stream_t *g_stream_stdin;
extern stream_t *g_stream_stdout;
extern stream_t *g_stream_stderr;

stream_t *fdstream_init(void *p, int fd);

#endif
