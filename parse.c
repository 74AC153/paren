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
		/* we consumed the entire string doing the conversion -- was a number */
		ret = node_new_value(val);
	} else {
		ret = node_new_symbol(tok_sym(tok));
	}

	next_tok(input, tok_list);

	*result = ret;
	return PARSE_OK;
}

static
parse_err_t parse_sexpr(char **input, token_t **tok_list, node_t **result);

static
parse_err_t parse_list(char **input, token_t **tok_list, node_t **result)
{
	node_t *child = NULL, *next = NULL, *ret = NULL;
	token_t *tok = first_tok(input, tok_list);
	parse_err_t status = PARSE_OK;

	if(tok_type(tok) == TOK_NONE) {
		status = PARSE_TOKEN_UNDERFLOW;
		goto finish;
	}
	/* check but don't consume this RPAREN because the parse_sexpr
	   that calls this function will check for it when this function
	   returns */
	else if(tok_type(tok) == TOK_RPAREN) {
		ret = NULL;
		goto finish;
	}

	status = parse_sexpr(input, tok_list, &child);
	if(status != PARSE_OK) {
		goto finish;
	}

	status = parse_list(input, tok_list, &next);
	if(status != PARSE_OK) {
		goto finish;
	}

	ret = node_new_list(child, next);
finish:
	node_release(child);
	node_release(next);
	*result = ret;

	return status;
}

static
parse_err_t parse_sexpr(char **input, token_t **tok_list, node_t **result)
{
	node_t *ret = NULL, *child = NULL, *next = NULL;
	token_t *tok;

	parse_err_t status = PARSE_OK;

	if(! (tok = first_tok(input, tok_list))) {
		goto underflow;
	}

	switch(tok_type(tok)) {
	case TOK_ATOM:
		status = parse_atom(input, tok_list, &ret);
		break;

	case TOK_LPAREN:
		/* ( ... */

		tok = next_tok(input, tok_list);
		if(! tok) {
			goto underflow;
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
		/* ( <sexpr> ... */
		status = parse_sexpr(input, tok_list, &child);

		tok = first_tok(input, tok_list);
		if(! tok) {
			goto underflow;
		}
		if(tok_type(tok) == TOK_DOT) {
			/* ( <sexpr> . <sexpr> */
			tok = next_tok(input, tok_list);
			if(! tok) {
				goto underflow;
			}
			if(tok_type(tok) == TOK_RPAREN) {
				status = PARSE_UNEXPECTED_RPAREN;
				goto error;
			}
			if(tok_type(tok) == TOK_DOT) {
				status = PARSE_UNEXPECTED_DOT;
				goto error;
			}
			status = parse_sexpr(input, tok_list, &next);
			if(status != PARSE_OK) {
				goto error;
			}
		} else {
			/* ( <sexpr> <sexpr>*  */
			status = parse_list(input, tok_list, &next);
			if(status != PARSE_OK) {
				goto error;
			}
		}
		ret = node_new_list(child, next);
		node_release(child);
		node_release(next);

		/* ... ) */
		tok = first_tok(input, tok_list);
		if(! tok) {
			goto underflow;
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
	node_release(child);
	node_release(ret);
	node_release(next);
	return status;

underflow:
	status = PARSE_TOKEN_UNDERFLOW;
	node_release(child);
	node_release(ret);
	node_release(next);
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
	return parse_err_messages[err];
}
