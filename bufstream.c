#include "bufstream.h"

int bufstream_readch(void *p)
{
	int ch;
	bufstream_t *bs = (bufstream_t *) p;

	if(bs->len == 0) return -1;
	ch = *(bs->cursor);
	bs->cursor++;
	bs->len--;
	return ch;
}

bufstream_t *bufstream_init(void *p, unsigned char *start, size_t len)
{
	bufstream_t *s = (bufstream_t *) p;
	if(s) {
		s->cursor = start;
		s->len = len;
	}
	return s;
}
