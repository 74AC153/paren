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



typedef int (*wr_stream_putch_fn)(void *p, unsigned char val);

typedef struct {
	wr_stream_putch_fn putch_cb;
	void *wr_stream_priv;
} wr_stream_t;

static inline wr_stream_t *wr_stream_init(
	void *p,
	wr_stream_putch_fn putch_cb,
	void *wr_stream_priv)
{
	wr_stream_t *ws = (wr_stream_t *) p;
	if(ws) {
		ws->putch_cb = putch_cb;
		ws->wr_stream_priv = wr_stream_priv;
	}
	return ws;
}

static inline int wrstream_putch(wr_stream_t *ws, unsigned char val)
{
	return ws->putch_cb(ws->wr_stream_priv, val);
}

#endif
