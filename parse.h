#ifndef PARSE_H
#define PARSE_H

#include <sys/types.h>
#include "node.h"
#include "stream.h"

#define PARSE_ERR_DEFS \
X(PARSE_OK, "no error") \
X(PARSE_UNEXPECTED_DOT, "unexpected '.'") \
X(PARSE_UNEXPECTED_RPAREN, "unexpected ')'") \
X(PARSE_TOKEN_UNDERFLOW, "token underflow") \
X(PARSE_EXPECTED_RPAREN, "expected ')'") \
X(PARSE_EXPECTED_ATOM, "expected atom") \
X(PARSE_EXPECTED_LPAREN_ATOM, "epxected '(' or atom") \
X(PARSE_END, "end of data")

typedef enum {
#define X(A, B) A,
PARSE_ERR_DEFS
#undef X
} parse_err_t;

typedef struct {
	tok_state_t tokstate;
} parse_state_t;

parse_state_t *parse_state_init(void *p, stream_t *stream);

parse_err_t parse(parse_state_t *state, node_t *out_hdl);

void parse_location(
	parse_state_t *state,
	off_t *offset,
	off_t *line,
	off_t *linechr);

char *parse_err_str(parse_err_t err);

#endif
