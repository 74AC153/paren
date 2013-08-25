#if ! defined(STREAM_H)
#define STREAM_H

/* return unsigned char value or -1 for end of data */
typedef int (*stream_fn)(void *p);

typedef struct {
	stream_fn getch_cb;
	void *getch_priv;
} stream_t;

static inline stream_t *stream_init(
	void *p,
	stream_fn getch_cb,
	void *getch_priv)
{
	stream_t *s = (stream_t *) p;
	if(s) {
		s->getch_cb = getch_cb;
		s->getch_priv = getch_priv;
	}
	return s;
}

static inline int stream_advance(stream_t *s)
{
	return s->getch_cb(s->getch_priv);
}

#endif
