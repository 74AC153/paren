#if ! defined(STREAM_H)
#define STREAM_H

/* return unsigned char value or -1 for end of data */
typedef int (*stream_getch_fn)(void *p);

typedef int (*stream_putch_fn)(void *p, unsigned char val);

typedef struct {
	void *stream_priv;
	stream_getch_fn getch_cb;
	stream_putch_fn putch_cb;
} stream_t;

static inline stream_t *stream_init(
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

static inline int stream_getch(stream_t *s)
{
	return s->getch_cb(s->stream_priv);
}

static inline int stream_putch(stream_t *s, unsigned char val)
{
	return s->putch_cb(s->stream_priv, val);
}

#endif
