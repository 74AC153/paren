#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "token.h"
#include "parse.h"

static
parse_err_t parse_atom(char **input, token_t **tok_list, node_t **result)
{
	node_t *ret = NULL;
	token_t *tok;
	char *end;
	uint64_t val;

	tok = first_tok(input, tok_list);

	if(tok_type(tok) != TOK_ATOM) {
		return PARSE_EXPECTED_ATOM;
	}

	val = strtoll(tok_sym(tok), &end, 0);
	if(! *end) {
		/* entire string consumed doing the conversion: number */
		ret = node_new_value(val);
	} else {
		ret = node_new_symbol(tok_sym(tok));
	}

	next_tok(input, tok_list);

	assert(ret);

	*result = node_retain(ret);

	return PARSE_OK;
}

static
parse_err_t parse_sexpr(char **input, token_t **tok_list, node_t **result);

static
parse_err_t parse_list(char **input, token_t **tok_list, node_t **result)
{
	token_t *tok = first_tok(input, tok_list);
	parse_err_t status = PARSE_OK;

	if(tok_type(tok) == TOK_NONE) {
		status = PARSE_TOKEN_UNDERFLOW;
		*result = NULL;
	} else if(tok_type(tok) == TOK_RPAREN) {
		/* ... ) */
		/* check but don't consume this RPAREN because the parse_sexpr
		   that calls this function will check for it when this function
		   returns */
		*result = NULL;
	} else if(tok_type(tok) == TOK_DOT) {
		tok = next_tok(input, tok_list);
		if(! tok) {
			status = PARSE_TOKEN_UNDERFLOW;
		} else if(tok_type(tok) == TOK_RPAREN) {
			status = PARSE_UNEXPECTED_RPAREN;
		} else if(tok_type(tok) == TOK_DOT) {
			status = PARSE_UNEXPECTED_DOT;
		} else {
			status = parse_sexpr(input, tok_list, result);
		}
	} else {
		node_t *child = NULL;

		status = parse_sexpr(input, tok_list, &child);
		if(status == PARSE_OK) {
			node_t *next = NULL;

			status = parse_list(input, tok_list, &next);

			if(status == PARSE_OK) {
				*result = node_retain(node_new_list(child, next));
			}

			node_release(next);
		}
		node_release(child);
	}

	return status;
}

static
parse_err_t parse_sexpr(char **input, token_t **tok_list, node_t **result)
{
	node_t *ret = NULL;
	token_t *tok;

	parse_err_t status = PARSE_OK;

	if(! (tok = first_tok(input, tok_list))) {
		return PARSE_TOKEN_UNDERFLOW;
	}

	switch(tok_type(tok)) {
	case TOK_ATOM:
		status = parse_atom(input, tok_list, &ret);
		break;

	case TOK_QUOTE: {
		node_t *val = NULL;
		/* consume quote and parse remaining */
		tok = next_tok(input, tok_list);
		status = parse_sexpr(input, tok_list, &val);
		if(status == PARSE_OK) {
			ret = node_retain(node_new_quote(val));
		}
		node_release(val);
		break;
	}
	case TOK_LPAREN:
		/* ( ... */

		tok = next_tok(input, tok_list);
		if(! tok) {
			status = PARSE_TOKEN_UNDERFLOW;
			goto error;
		}
		if(tok_type(tok) == TOK_RPAREN) {
			/* ( ) */
			/* consume closing rparen */
			next_tok(input, tok_list);
			*result = NULL;
			break;
		} else if(tok_type(tok) == TOK_DOT) {
			/* ( .  */
			status = PARSE_UNEXPECTED_DOT;
			goto error;
		}
		/* ( ... */
		status = parse_list(input, tok_list, &ret);

		/* ... ) */
		tok = first_tok(input, tok_list);
		if(! tok) {
			status = PARSE_TOKEN_UNDERFLOW;
			goto error;
		}
		if(tok_type(tok) != TOK_RPAREN) {
			status = PARSE_EXPECTED_RPAREN;
			goto error;
		}
		/* consume closing rparen */
		next_tok(input, tok_list);
		break;

	default:
		status = PARSE_EXPECTED_LPAREN_ATOM;
		goto error;
	}

	*result = ret;
	return status;

error:
	node_release(ret);
	return status;
}

parse_err_t parse(char *input, char **remain, node_t **result)
{
	parse_err_t status = PARSE_OK;
	token_t *tok_list = NULL;

	status = parse_sexpr(&input, &tok_list, result);

	*remain = input;

	return status;
}

static char *parse_err_messages[] = {
#define X(A, B) B,
PARSE_ERR_DEFS
#undef X
};

char *parse_err_str(parse_err_t err)
{
	if(err >= sizeof(parse_err_messages) / sizeof(parse_err_messages[0])) {
		return "unknown error";
	}
	return parse_err_messages[err];
}
