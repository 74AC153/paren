#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include "stream.h"

#define MAX_SYM_LEN 32

#define TOK_TYPES \
X(TOK_INIT) \
X(TOK_LPAREN) \
X(TOK_RPAREN) \
X(TOK_DOT) \
X(TOK_SYM) \
X(TOK_LIT) \
X(TOK_END)

typedef enum {
#define X(name) name,
TOK_TYPES
#undef X
} toktype_t;

typedef struct {
	toktype_t type;
	bool in_string;
	off_t stream_off, stream_lines, stream_linechar;
	union {
		unsigned char sym[MAX_SYM_LEN+1]; // include space for trailing null
		int64_t lit;
	} u;

	int nextch;
	stream_t *stream;
} tok_state_t;

tok_state_t *tok_state_init(void *p, stream_t *stream);

void token_chomp(tok_state_t *state);

toktype_t token_type(tok_state_t *state);

char *token_sym(tok_state_t *state);
int64_t token_lit(tok_state_t *state);

char *token_type_str(tok_state_t *state);
void token_print(tok_state_t *state);

off_t tok_state_offset(tok_state_t *state);
off_t tok_state_line(tok_state_t *state);
off_t tok_state_linechr(tok_state_t *state);


/* for your convenience */

typedef struct {
	unsigned char *cursor;
	size_t len;
} bufstream_t;

int bufstream_readch(void *p);
bufstream_t *bufstream_init(void *p, unsigned char *start, size_t len);

#endif
