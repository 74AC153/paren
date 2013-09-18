#if ! defined(STREAM_H)
#define STREAM_H

#include <stddef.h>

#define STREAM_END (-1)

/* return 0 for success, STREAM_END for EOF, nonzero for other errors */
typedef int (*stream_getch_fn)(void *p, char *ch_out);

/* return nonzero for error */
typedef int (*stream_putch_fn)(void *p, char ch_in);

typedef struct {
	void *stream_priv;
	stream_getch_fn getch_cb;
	stream_putch_fn putch_cb;
	int status;
} stream_t;

stream_t *stream_init(
	void *p,
	stream_getch_fn getch_cb,
	stream_putch_fn putch_cb,
	void *stream_priv);

/* return -1 for error (including end of stream) */
int stream_getch(stream_t *s, char *ch_out);

/* return -1 for error (including end of stream) */
int stream_putch(stream_t *s, char ch_in);

/* return # chars written or -1 for error */
int stream_putstr(stream_t *s, char *str);

/* return # chars written or -1 for error */
int stream_putcomp(stream_t *s, ...);

int stream_status_get(stream_t *s);

_Bool stream_is_end(stream_t *s);

#define STREAM_STATIC_INITIALIZER(PRIV, GETCH, PUTCH) \
{ \
	.stream_priv = (PRIV), \
	.getch_cb = (GETCH), \
	.putch_cb = (PUTCH), \
	.status = 0 \
}

#endif
