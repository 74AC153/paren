#ifndef PARSE_H
#define PARSE_H

#include "node.h"

#define PARSE_ERR_DEFS \
X(PARSE_OK, "no error") \
X(PARSE_UNEXPECTED_DOT, "unexpected '.'") \
X(PARSE_UNEXPECTED_RPAREN, "unexpected ')'") \
X(PARSE_TOKEN_UNDERFLOW, "token underflow") \
X(PARSE_EXPECTED_RPAREN, "expected ')'") \
X(PARSE_EXPECTED_ATOM, "expected atom") \
X(PARSE_EXPECTED_LPAREN_ATOM, "epxected '(' or atom")

typedef enum {
#define X(A, B) A,
PARSE_ERR_DEFS
#undef X
} parse_err_t;

parse_err_t parse(char *input, char **remain, node_t **result);

char *parse_err_str(parse_err_t err);

#endif
