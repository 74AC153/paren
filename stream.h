#if ! defined(STREAM_H)
#define STREAM_H

#include <stddef.h>

/* return unsigned char value or -1 for end of data */
typedef int (*stream_getch_fn)(void *p);

/* return 1 for success, -1 for error */
typedef int (*stream_putch_fn)(void *p, char val);

typedef struct {
	void *stream_priv;
	stream_getch_fn getch_cb;
	stream_putch_fn putch_cb;
} stream_t;

stream_t *stream_init(
	void *p,
	stream_getch_fn getch_cb,
	stream_putch_fn putch_cb,
	void *stream_priv);

int stream_getch(stream_t *s);

int stream_putch(stream_t *s, char val);

int stream_putstr(stream_t *s, char *str, size_t *nput);

int stream_putcomp(stream_t *s, size_t *nput, ...);

#define STREAM_STATIC_INITIALIZER(PRIV, GETCH, PUTCH) \
	{ .stream_priv = (PRIV), .getch_cb = (GETCH), .putch_cb = (PUTCH) }

#endif
