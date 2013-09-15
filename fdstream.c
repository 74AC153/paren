#include <unistd.h>
#include "fdstream.h"

static int fdstream_writech(void *p, char c)
{
	fdstream_t *fs = (fdstream_t *) p;

	return write(fs->fd, &c, 1);
}

static int fdstream_readch(void *p)
{
	unsigned char c;
	int status;
	fdstream_t *fs = (fdstream_t *) p;
	
	status = read(fs->fd, &c, 1);
	if(status == 0) {
		return -1;
	}

	return (int) c;
}

stream_t *fdstream_init(void *p, int fd)
{
	fdstream_t *fs = (fdstream_t *) p;
	if(fs) {
		fs->fd = fd;
		stream_init(&(fs->hdr), fdstream_readch, fdstream_writech, fs);
	}
	return &(fs->hdr);
}

