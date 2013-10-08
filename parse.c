#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "token.h"
#include "parse.h"

typedef struct {
	memory_state_t *ms;
	tok_state_t *ts;
} parse_state_t;

static
parse_err_t parse_atom(parse_state_t *state, node_t **result)
{
	switch(token_type(state->ts)) {
	case TOK_SYM:
		*result = node_symbol_new(state->ms, token_sym(state->ts));
		break;

	case TOK_LIT:
		*result = node_value_new(state->ms, token_lit(state->ts));
		break;

	default:
		return PARSE_EXPECTED_ATOM;
	}
	assert(*result);

	token_chomp(state->ts);

	return PARSE_OK;
}

static
parse_err_t parse_sexpr(parse_state_t *state, node_t **result);

static
parse_err_t parse_list(parse_state_t *state, node_t **result)
{
	parse_err_t status = PARSE_OK;
	node_t *child = NULL, *next = NULL;

	switch(token_type(state->ts)) {
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
		token_chomp(state->ts);
		switch(token_type(state->ts)) {
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
				*result = node_cons_new(state->ms, child, next);
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

	parse_err_t status = PARSE_OK;

	switch(token_type(state->ts)) {
	case TOK_END:
		return PARSE_END;

	case TOK_SYM:
	case TOK_LIT:
		status = parse_atom(state, &ret);
		break;

	case TOK_LPAREN:
		/* ( ... */

		token_chomp(state->ts);
		
		switch(token_type(state->ts)) {
		case TOK_END:
			status = PARSE_TOKEN_UNDERFLOW;
			goto error;

		case TOK_RPAREN:
			/* ( ) */
			/* consume closing rparen */
			token_chomp(state->ts);
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
		switch(token_type(state->ts)) {
		case TOK_END:
			status = PARSE_TOKEN_UNDERFLOW;
			goto error;
		case TOK_RPAREN:
			/* consume closing rparen */
			token_chomp(state->ts);
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

parse_err_t parse(memory_state_t *ms, tok_state_t *ts, node_t *out_hdl)
{
	parse_err_t status = PARSE_OK;
	node_t *result = NULL;
	parse_state_t state = { .ms = ms, .ts = ts };

	if(token_type(state.ts) == TOK_INIT) {
		token_chomp(state.ts);
	}
	status = parse_sexpr(&state, &result);
	node_handle_update(out_hdl, result);

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
