#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "token.h"
#include "parse.h"

static node_t *parse_atom(char **input, token_t **tok_list)
{
	node_t *ret;
	token_t *tok;
	char *end;
	uint64_t val;

	tok = first_tok(input, tok_list);

	if(tok_type(tok) != TOK_ATOM) {
		fprintf(stderr, "unexpected token: %s\n", tok_type_str(tok));
		return NULL;
	}

	val = strtoll(tok_sym(tok), &end, 0);
	if(! *end) {
		/* we consumed the entire string doing the conversion -- was a number */
		ret = node_new_value(val);
	} else {
		ret = node_new_symbol(tok_sym(tok));
	}

	next_tok(input, tok_list);
	return ret;
}

static node_t *parse_sexpr(char **input, token_t **tok_list);

static node_t *parse_list(char **input, token_t **tok_list)
{
	node_t *child, *next, *ret;
	token_t *tok = first_tok(input, tok_list);

	if(tok_type(tok) == TOK_NONE) {
		return NULL;
	}
	/* check but don't consume this RPAREN because the parse_sexpr
	   that calls this function will check for it when this function
	   returns */
	else if(tok_type(tok) == TOK_RPAREN) {
		return NULL;
	}
	child = parse_sexpr(input, tok_list);
	next = parse_list(input, tok_list);
	ret = node_new_list(child, next);
	node_release(child);
	node_release(next);
	return ret;
}

static node_t *parse_sexpr(char **input, token_t **tok_list)
{
	node_t *ret = NULL, *child = NULL, *next = NULL;
	token_t *tok;

	if(! (tok = first_tok(input, tok_list))) {
		goto underflow;
	}

	switch(tok_type(tok)) {
	case TOK_ATOM:
		ret = parse_atom(input, tok_list);
		break;

	case TOK_LPAREN:
		/* ( ... */
		if(! (tok = next_tok(input, tok_list))) {
			goto underflow;
		}
		if(tok_type(tok) == TOK_RPAREN) {
			/* ( ) */
			/* consume closing rparen */
			next_tok(input, tok_list);
			ret = NULL;
			break;
		} else if(tok_type(tok) == TOK_DOT) {
			/* ( .  */
			goto error;
		}
		/* ( <sexpr> ... */
		child = parse_sexpr(input, tok_list);

		if(! (tok = first_tok(input, tok_list))) {
			goto underflow;
		}
		if(tok_type(tok) == TOK_DOT) {
			/* ( <sexpr> . <sexpr> */
			if(! (tok = next_tok(input, tok_list))) {
				goto underflow;
			}
			if(tok_type(tok) == TOK_RPAREN || tok_type(tok) == TOK_DOT) {
				goto error;
			}
			next = parse_sexpr(input, tok_list);
		} else {
			/* ( <sexpr> <sexpr>*  */
			next = parse_list(input, tok_list);
		}
		ret = node_new_list(child, next);
		node_release(child);
		node_release(next);

		/* ... ) */
		if(! (tok = first_tok(input, tok_list))) {
			goto underflow;
		}
		if(tok->type != TOK_RPAREN) {
			goto error;
		}
		/* consume closing rparen */
		next_tok(input, tok_list);
		break;

	default:
		goto error;
	}

	return ret;

error:
	fprintf(stderr, "unexpected %s before %40s...\n",
	        tok_type_str(tok), *input);
	node_release(child);
	node_release(ret);
	node_release(next);
	return NULL;

underflow:
	fprintf(stderr, "unexpected end of input\n");
	node_release(child);
	node_release(ret);
	node_release(next);
	return NULL;
}

node_t *parse(char *input, char **remain)
{
	node_t *ret;
	token_t *tok_list = NULL;

	ret = parse_sexpr(&input, &tok_list);

	*remain = input;

	return ret;
}
