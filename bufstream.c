#include "bufstream.h"

static int bufstream_readch(void *p, char *ch_out)
{
	int ch;
	bufstream_t *bs = (bufstream_t *) p;

	if(bs->roff == bs->len) {
		return STREAM_END;
	}
	*ch_out = bs->buf[bs->roff++];
	return 0;
}

stream_t *bufstream_init(void *p, char *start, size_t len)
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

