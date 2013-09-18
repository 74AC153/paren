#include <stdarg.h>
#include <limits.h>
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
		s->status = 0;
	}
	return s;
}

int stream_getch(stream_t *s, char *ch_out)
{
	s->status = s->getch_cb(s->stream_priv, ch_out);
	if(s->status) {
		return -1;
	}
	return 0;
}

int stream_putch(stream_t *s, char val)
{
	int status;
	s->status = s->putch_cb(s->stream_priv, val);
	if(status) {
		return -1;
	}
	return 0;
}

int stream_putstr(stream_t *s, char *str)
{
	int status = 0;
	int count = 0;
	while(*str) {
		if(count == INT_MAX) {
			break;
		}

		s->status = s->putch_cb(s->stream_priv, *str);

		if(s->status < 0) {
			if(count == 0) {
				count = -1;
			}
			break;
		}
		str++;
		count++;
	}
	return count;
}

int stream_putcomp(stream_t *s, ...)
{
	va_list ap;
	int status = 0;
	size_t count = 0;
	char *str;

	va_start(ap, s);

	while(NULL != (str = va_arg(ap, char *))) {
		while(*str) {
			if(count == INT_MAX) {
				goto finish;
			}

			s->status = s->putch_cb(s->stream_priv, *str);

			if(status < 0) {
				if(count == 0) {
					count = -1;
					goto finish;
				}
			}

			str++;
			count++;
		}
	}

finish:
	va_end(ap);
	return count;
}

int stream_status_set(stream_t *s)
{
	return s->status;
}

_Bool stream_is_end(stream_t *s)
{
	return s->status == STREAM_END;
}
