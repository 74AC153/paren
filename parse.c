#include <stdlib.h>
#include <assert.h>

#include "token.h"
#include "parse.h"

parse_state_t *parse_state_init(void *p, stream_t *stream)
{
	parse_state_t *s = (parse_state_t *) p;
	if(s) {
		tok_state_init(&(s->tokstate), stream);
	}
	return s;
}

static
parse_err_t parse_atom(parse_state_t *state, node_t **result)
{
	tok_state_t *tokstate = &(state->tokstate);

	switch(token_type(tokstate)) {
	case TOK_SYM:
		*result = node_symbol_new(token_sym(tokstate));
		break;

	case TOK_LIT:
		*result = node_value_new(token_lit(tokstate));
		break;

	default:
		return PARSE_EXPECTED_ATOM;
	}
	assert(*result);

	token_chomp(tokstate);

	return PARSE_OK;
}

static
parse_err_t parse_sexpr(parse_state_t *state, node_t **result);

static
parse_err_t parse_list(parse_state_t *state, node_t **result)
{
	parse_err_t status = PARSE_OK;
	node_t *child = NULL, *next = NULL;
	tok_state_t *tokstate = &(state->tokstate);

	switch(token_type(tokstate)) {
	case TOK_END:
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
		token_chomp(tokstate);
		switch(token_type(tokstate)) {
		case TOK_END:
			status = PARSE_TOKEN_UNDERFLOW;
			break;
		case TOK_RPAREN:
			status = PARSE_UNEXPECTED_RPAREN;
			break;
		case TOK_DOT:
			status = PARSE_UNEXPECTED_DOT;
			break;
		default:
			status = parse_sexpr(state, result);
			break;
		}
		break;
	default:
		status = parse_sexpr(state, &child);
		if(status == PARSE_OK) {
			status = parse_list(state, &next);
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
parse_err_t parse_sexpr(parse_state_t *state, node_t **result)
{
	node_t *ret = NULL;
	tok_state_t *tokstate = &(state->tokstate);

	parse_err_t status = PARSE_OK;

	switch(token_type(tokstate)) {
	case TOK_END:
		return PARSE_END;

	case TOK_SYM:
	case TOK_LIT:
		status = parse_atom(state, &ret);
		break;

	case TOK_LPAREN:
		/* ( ... */

		token_chomp(tokstate);
		
		switch(token_type(tokstate)) {
		case TOK_END:
			status = PARSE_TOKEN_UNDERFLOW;
			goto error;

		case TOK_RPAREN:
			/* ( ) */
			/* consume closing rparen */
			token_chomp(tokstate);
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
		status = parse_list(state, &ret);

		/* ... ) */
		switch(token_type(tokstate)) {
		case TOK_END:
			status = PARSE_TOKEN_UNDERFLOW;
			goto error;
		case TOK_RPAREN:
			/* consume closing rparen */
			token_chomp(tokstate);
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

parse_err_t parse(parse_state_t *state, node_t *out_hdl)
{
	parse_err_t status = PARSE_OK;
	node_t *result = NULL;

	if(token_type(&(state->tokstate)) == TOK_INIT) {
		token_chomp(&(state->tokstate));
	}
	status = parse_sexpr(state, &result);
	node_handle_update(out_hdl, result);

	return status;
}

void parse_location(
	parse_state_t *state,
	off_t *offset,
	off_t *line,
	off_t *linechr)
{
	if(offset) *offset = tok_state_offset(&(state->tokstate));
	if(line) tok_state_line(&(state->tokstate));
	if(linechr) tok_state_linechr(&(state->tokstate));
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
