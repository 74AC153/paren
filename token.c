#include <assert.h>
#include <sys/types.h>
#include <stdbool.h>

#include "token.h"
#include "stream.h"
#include "libc_custom.h"

char *token_type_names[] = {
	#define X(name) #name,
	TOK_TYPES
	#undef X
};

tok_state_t *tok_state_init(void *p, stream_t *stream)
{
	tok_state_t *s = (tok_state_t *)p;
	if(s) {
		s->type = TOK_INIT;
		s->in_string = false;
		s->stream_off = 0;
		s->stream_lines = 1;
		s->stream_linechar = 0;
		s->nextch = -1;
		s->stream = stream;
	}
	return s;
}

off_t tok_state_offset(tok_state_t *state)
{
	return state->stream_off;
}

off_t tok_state_line(tok_state_t *state)
{
	return state->stream_lines;
}

off_t tok_state_linechr(tok_state_t *state)
{
	return state->stream_linechar;
}

static bool isspace_custom(char c)
{
	return c == '\t'
	    || c == '\n'
	    || c == '\v'
	    || c == '\f'
	    || c == '\r'
	    || c == ' ';
}
static bool is_atom_char(char c)
{
	return !isspace_custom(c) && c != '(' && c != ')' && c != '.' && c != '\'';
}

static void advance_tok_stream(tok_state_t *s)
{
	int status;
	char ch;
	status = stream_getch(s->stream, &ch);
	if(status == 0) {
		s->nextch = ch;
		s->stream_off++;
		if(s->nextch == '\n') {
			s->stream_lines++;
			s->stream_linechar = 0;
		}
		s->stream_linechar++;
	} else {
		/* ostensibly, the stream is EOF, but... FIXME */
		s->nextch = -1;
	}
}

void token_chomp(tok_state_t *state)
{
	size_t len;
	int64_t lit;
	bool in_comment = false;
	char *cursor, *test_end;

	/* prime the stream (no-op if stream end) */
	if(state->nextch == -1) {
		state->in_string = false;
		advance_tok_stream(state);
	}

	/* string-mode shortcut */
	if(state->in_string) {
		if(state->nextch == '"') {
			state->in_string = false;
			state->type = TOK_RPAREN;
			goto finish;
		}

		state->type = TOK_LIT;

		if(state->nextch != '\\') {
			state->u.lit = state->nextch;
			goto finish;
		}
		
		/* \n    0x0A newline
		   \r    0x0D carriage return
		   \t    0x09 tab
		   \###  octal value
		   \anything else -> anything else */
		advance_tok_stream(state); // consume '\'
		switch(state->nextch) {
		case -1:
			state->u.lit = '\\';
			state->in_string = false;
			break;
		case 'n': state->u.lit = 0xA; break;
		case 'r': state->u.lit = 0xD; break;
		case 't': state->u.lit = 0x9; break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			state->u.lit = state->nextch - '0';
			advance_tok_stream(state);
			while(state->nextch >= '0' && state->nextch <= '7') {
				state->u.lit = (state->u.lit << 3) | (state->nextch - '0');
				advance_tok_stream(state); //consume next digit
			}
			return; // early return because we've been doing lookahead
		default: state->u.lit = state->nextch; break;
		}
		goto finish;
	}

	/* skip leading whitespace and comments */
	while(true) {
		if(state->nextch == '#') {
			in_comment = true;
		} else if(state->nextch == '\n') {
			in_comment = false;
		}
		if(! isspace_custom(state->nextch) && ! in_comment) {
			break;
		}
		advance_tok_stream(state);
	}

	switch(state->nextch) {
	case -1: /* end of input */
		state->type = TOK_END;
		break;
	case '"':
		state->type = TOK_LPAREN;
		state->in_string = true;
		break;
	case '(':
		state->type = TOK_LPAREN;
		break;
	case ')':
		state->type = TOK_RPAREN;
		break;
	case '.':
		state->type = TOK_DOT;
		break;
	default:
		/* read stream into symbol buffer and null-terminate */
		for(cursor = state->u.sym;
		    (ssize_t)(cursor-state->u.sym) < (ssize_t)(sizeof(state->u.sym)-1)
		     && is_atom_char(state->nextch);
		    cursor++) {
			if(state->nextch == -1) break;
			*cursor = (unsigned char) state->nextch;
			advance_tok_stream(state);
		}
		*cursor = 0; /* null terminate u.sym */
		/* discard rest of token that doesn't fit in symbol buffer */
		if(cursor - state->u.sym == (sizeof(state->u.sym) - 1)) {
			while(is_atom_char(state->nextch)) {
				advance_tok_stream(state);
			}
		}

		/* try to convert symbol buffer to a numeric literal */
		lit = strtoll_custom((char *) state->u.sym, (char **) &test_end);

		if(test_end == cursor) {
			/* whole string consumed -> numeric literal */
			state->type = TOK_LIT;
			state->u.lit = lit;
		} else {
			/* not numeric -> symbol */
			state->type = TOK_SYM;
		}
		return; // early return because we've been doing lookahead
	}
finish:
	advance_tok_stream(state);
}

toktype_t token_type(tok_state_t *state)
{
	return state->type;
}

char *token_sym(tok_state_t *state)
{
	assert(state->type == TOK_SYM);
	return (char *) state->u.sym;
}

int64_t token_lit(tok_state_t *state)
{
	assert(state->type == TOK_LIT);
	return state->u.lit;
}

char *token_type_str(tok_state_t *state)
{
	return token_type_names[state->type];
}

void token_print(stream_t *s, tok_state_t *state)
{
	char buf[17];
	switch(state->type) {
	case TOK_SYM: 
		stream_put(s, "TOK_SYM(", state->u.sym, ")\n", NULL);
		break;
	case TOK_LIT:
		stream_put(s, "TOK_LIT(0x", fmt_u64(buf, state->u.lit), ")\n", NULL);
		break;
	default:
		stream_put(s, token_type_names[state->type], "\n", NULL);
		break;
	}
}
