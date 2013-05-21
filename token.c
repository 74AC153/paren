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

static bool is_atom_char(char c)
{
	return !isspace(c) && c != '(' && c != ')' && c != '.' && c != '\'';
}

static size_t read_tok(char *input, tok_state_t *state)
{
	char *start, *end, *test_end;
	size_t len;
	int64_t lit;

	/* skip leading whitespace */
	for(start = input; isspace(start[0]); start++);

	switch(start[0]) {
	case 0: /* end of input */
		state->type = TOK_NONE;
		end = start;
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
	case '\'':
		state->type = TOK_QUOTE;
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
