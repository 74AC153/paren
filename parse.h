#ifndef PARSE_H
#define PARSE_H

#include <sys/types.h>
#include "node.h"
#include "stream.h"
#include "memory.h"

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

parse_err_t parse(memory_state_t *ms, tok_state_t *ts, node_t *out_hdl);

char *parse_err_str(parse_err_t err);

#endif
