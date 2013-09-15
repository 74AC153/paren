#include <stdarg.h>
#include "stream.h"

stream_t *stream_init(
	void *p,
	stream_getch_fn getch_cb,
	stream_putch_fn putch_cb,
	void *stream_priv)
{
	stream_t *s = (stream_t *) p;
	if(s) {
		s->getch_cb = getch_cb;
		s->putch_cb = putch_cb;
		s->stream_priv = stream_priv;
	}
	return s;
}

int stream_getch(stream_t *s)
{
	return s->getch_cb(s->stream_priv);
}

int stream_putch(stream_t *s, char val)
{
	return s->putch_cb(s->stream_priv, val);
}

int stream_putstr(stream_t *s, char *str, size_t *nput)
{
	int status = 0;
	size_t count = 0;
	while(*str) {
		status = stream_putch(s, *str++);
		if(status < 0) break;
		count++;
	}
	if(nput) *nput = count;
	return status;
}

int stream_putcomp(stream_t *s, size_t *nput, ...)
{
	va_list ap;
	int status = 0;
	size_t total = 0, count;
	char *str;

	va_start(ap, nput);

	while(NULL != (str = va_arg(ap, char *))) {
		status = stream_putstr(s, str, &count);
		if(status < 0) break;
		total += count;
	}
	if(nput) *nput = total;

	va_end(ap);
	return status;
}
