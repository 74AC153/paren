#include "bufstream.h"

static int bufstream_readch(void *p)
{
	int ch;
	bufstream_t *bs = (bufstream_t *) p;

	if(bs->roff == bs->len) return -1;
	ch = bs->buf[bs->roff++];
	return ch;
}

stream_t *bufstream_init(void *p, unsigned char *start, size_t len)
{
	bufstream_t *bs = (bufstream_t *) p;
	if(bs) {
		bs->buf = start;
		bs->len = len;
		bs->roff = 0;
		stream_init(&(bs->hdr), bufstream_readch, NULL, bs);
	}
	return &(bs->hdr);
}

