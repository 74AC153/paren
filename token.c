#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "token.h"

char *token_type_names[] = {
	#define X(name) #name,
	TOK_TYPES
	#undef X
};

tok_state_t *tok_state_init(void *p)
{
	tok_state_t *s = (tok_state_t *)p;
	if(s) {
		s->type = TOK_NONE;
		s->in_string = false;
	}
	return s;
}

static bool is_atom_char(char c)
{
	return !isspace(c) && c != '(' && c != ')' && c != '.' && c != '\'';
}

static size_t string_mode(char *input, tok_state_t *state)
{
	if(input[0] == 0) {
		state->type = TOK_NONE;
		return 0;
	}

	if(input[0] == '"') {
		state->type = TOK_RPAREN;
		state->in_string = false;
		return 1;
	}

	state->type = TOK_LIT;

	if(input[0] != '\\' || input[1] == 0) {
		state->u.lit = input[0];
		return 1;
	}
	
	/* \n   0x0A newline
	   \r   0x0D carriage return
	   \t   0x09 tab
	   \x## 0x## 8 bit ASCII hex ##
	   \"   0x22 double quote
	   \\   0x5C backslash */
	switch(input[1]) {
	case 'n': state->u.lit = 0xA; return 2;
	case 'r': state->u.lit = 0xD; return 2;
	case 't': state->u.lit = 0x9; return 2;
	case 'x': state->u.lit = strtol(input+2, NULL, 16); return 4;
	case '"': state->u.lit = 0x22; return 2;
	case '\\': state->u.lit = 0x5C; return 2;
	default: state->u.lit = input[1]; break;
	}

	return 2;
}

static size_t read_tok(char *input, tok_state_t *state)
{
	char *start, *end, *test_end;
	size_t len;
	int64_t lit;

	/* string-mode shortcut */
	if(state->in_string) {
		return string_mode(input, state);
	}

	/* skip leading whitespace */
	for(start = input; isspace(start[0]); start++);

	switch(start[0]) {
	case 0: /* end of input */
		state->type = TOK_NONE;
		end = start;
		break;
	case '"':
		state->type = TOK_LPAREN;
		state->in_string = true;
		end = start + 1;
		break;
	case '(':
		state->type = TOK_LPAREN;
		end = start + 1;
		break;
	case ')':
		state->type = TOK_RPAREN;
		end = start + 1;
		break;
	case '.':
		state->type = TOK_DOT;
		end = start + 1;
		break;
	default:
		/* determine where token ends */
		for(end = start; *end && is_atom_char(*end); end++);

		/* try to convert to a numeric literal */
		lit = strtoll(start, &test_end, 0);

		if(test_end == end) {
			/* whole string consumed -> numeric literal */
			state->type = TOK_LIT;
			state->u.lit = lit;
		} else {
			/* not numeric -> symbol */
			state->type = TOK_SYM;
			len = end - start;
			/* capture as many characters as we have space for, dispose rest */
			if(len > sizeof(state->u.sym)-1) {
				len = sizeof(state->u.sym)-1;
			}
			strncpy(state->u.sym, start, len);
			state->u.sym[len] = 0;
		}
		break;
	}

	return end - input;
}

void token_chomp(char **input, tok_state_t *state)
{
	size_t n;

	n = read_tok(*input, state);
	*input += n;
}

toktype_t token_type(tok_state_t *state)
{
	return state->type;
}

char *token_sym(tok_state_t *state)
{
	assert(state->type == TOK_SYM);
	return state->u.sym;
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

void token_print(tok_state_t *state)
{
	switch(state->type) {
	case TOK_SYM:
		printf("TOK_SYM(%s)\n", state->u.sym);
		break;
	case TOK_LIT:
		printf("TOK_LIT(0x%llx)\n", (unsigned long long) state->u.lit);
		break;
	default:
		printf("%s\n", token_type_names[state->type]);
		break;
	}
}
