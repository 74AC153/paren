#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "token.h"
#include "parse.h"


static
parse_err_t parse_atom(char **input, tok_state_t *state, node_t **result)
{
	switch(token_type(state)) {
	case TOK_SYM:
		*result = node_symbol_new(token_sym(state));
		break;

	case TOK_LIT:
		*result = node_value_new(token_lit(state));
		break;

	default:
		return PARSE_EXPECTED_ATOM;
	}
	assert(*result);

	token_chomp(input, state);

	return PARSE_OK;
}

static
parse_err_t parse_sexpr(char **input, tok_state_t *state, node_t **result);

static
parse_err_t parse_list(char **input, tok_state_t *state, node_t **result)
{
	parse_err_t status = PARSE_OK;
	node_t *child = NULL, *next = NULL;

	switch(token_type(state)) {
	case TOK_NONE:
		status = PARSE_TOKEN_UNDERFLOW;
		*result = NULL;
		break;

	case TOK_RPAREN:
		/* ... ) */
		/* check but don't consume this RPAREN because the parse_sexpr
		   that calls this function will check for it when this function
		   returns */
		*result = NULL;
		break;

	case TOK_DOT:
		token_chomp(input, state);
		switch(token_type(state)) {
		case TOK_NONE:
			status = PARSE_TOKEN_UNDERFLOW;
			break;
		case TOK_RPAREN:
			status = PARSE_UNEXPECTED_RPAREN;
			break;
		case TOK_DOT:
			status = PARSE_UNEXPECTED_DOT;
			break;
		default:
			status = parse_sexpr(input, state, result);
			break;
		}
		break;
	default:
		status = parse_sexpr(input, state, &child);
		if(status == PARSE_OK) {
			status = parse_list(input, state, &next);
			if(status == PARSE_OK) {
				*result = node_cons_new(child, next);
			} else {
				node_droproot(next);
			}
		}

	}

	return status;
}

static
parse_err_t parse_sexpr(char **input, tok_state_t *state, node_t **result)
{
	node_t *ret = NULL;

	parse_err_t status = PARSE_OK;

	switch(token_type(state)) {
	case TOK_NONE:
		return PARSE_OK;

	case TOK_SYM:
	case TOK_LIT:
		status = parse_atom(input, state, &ret);
		break;

	case TOK_LPAREN:
		/* ( ... */

		token_chomp(input, state);
		
		switch(token_type(state)) {
		case TOK_NONE:
			status = PARSE_TOKEN_UNDERFLOW;
			goto error;

		case TOK_RPAREN:
			/* ( ) */
			/* consume closing rparen */
			token_chomp(input, state);
			*result = NULL;
			goto tok_lparen_finish;

		case TOK_DOT:
			/* ( .  */
			status = PARSE_UNEXPECTED_DOT;
			goto error;

		default:
			break;
		}

		/* ( ... */
		status = parse_list(input, state, &ret);

		/* ... ) */
		switch(token_type(state)) {
		case TOK_NONE:
			status = PARSE_TOKEN_UNDERFLOW;
			goto error;
		case TOK_RPAREN:
			/* consume closing rparen */
			token_chomp(input, state);
			break;
		default:
			status = PARSE_EXPECTED_RPAREN;
			goto error;
		}

tok_lparen_finish:
		break;

	default:
		status = PARSE_EXPECTED_LPAREN_ATOM;
		goto error;
	}

	*result = ret;
	return status;

error:
	node_droproot(ret);
	return status;
}

parse_err_t parse(char *input, char **remain, node_t **result, parseloc_t *loc)
{
	parse_err_t status = PARSE_OK;
	tok_state_t state;

	(void) loc;
	tok_state_init(&state);

	/* prime tokenizer to have first token ready */
	token_chomp(&input, &state);

	*result = NULL;
	status = parse_sexpr(&input, &state, result);
	if(status != PARSE_OK) {
		node_droproot(*result);
	}

	*remain = input;
	/* the tokenizer will consume one token past what we've actually parsed,
	   so put the string for that last token back to where it should be.
	   However... if the tokenizer didn't consume any tokens, we don't need to
	   back up */ 
	if(token_type(&state) != TOK_NONE) {
		*remain -= token_lastchomp(&state);
	}

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
