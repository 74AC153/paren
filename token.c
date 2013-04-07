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
	return !isspace(c) && c != '(' && c != ')' && c != '.';
}

static size_t read_tok(char *input, token_t *tok_out)
{
	char *start, *end;
	size_t len;

	for(start = input; isspace(start[0]); start++);

	switch(start[0]) {
	case 0: /* end of input */
		tok_out->type = TOK_NONE;
		end = start;
		break;
	case '(':
		tok_out->type = TOK_LPAREN;
		end = start + 1;
		break;
	case ')':
		tok_out->type = TOK_RPAREN;
		end = start + 1;
		break;
	case '.':
		tok_out->type = TOK_DOT;
		end = start + 1;
		break;
	default:
		tok_out->type = TOK_ATOM;
		for(end = start; *end && is_atom_char(*end); end++);
		len = end - start;
		if(len > sizeof(tok_out->sym)) {
			len = sizeof(tok_out->sym) -1;
		}
		strncpy(tok_out->sym, start, len);
		tok_out->sym[len] = 0;
		break;
	}
	return end - input;
}

/* like strdup */
void *memdup(void *src, size_t len)
{
	void *ret = malloc(len);
	if(ret) {
		memcpy(ret, src, len);
	}
	return ret;
}

token_t *first_tok(char **input, token_t **tok_list)
{
	token_t temp = { 0, TOK_NONE, { 0 } }, *ret = NULL;
	size_t n;
	if(*tok_list) {
		ret = (*tok_list);
	} else {
		if((n = read_tok((*input), &temp))) {
			if((ret = memdup(&temp, sizeof(temp)))) {
				*tok_list = ret;
			}
		}
		(*input) += n;
	}
	return ret;
}

token_t *next_tok(char **input, token_t **tok_list)
{
	token_t *del = *tok_list;
	if(del) {
		*tok_list = (*tok_list)->next;
		free(del);
	}
	return first_tok(input, tok_list);
}

toktype_t tok_type(token_t *tok)
{
	if(tok) {
		return tok->type;
	}
	return TOK_NONE;
}

char *tok_type_str(token_t *tok)
{
	return token_type_names[tok->type];
}

char *tok_sym(token_t *tok)
{
	return tok->sym;
}

void push_tok(token_t *tok, token_t **tok_list)
{
	tok->next = (*tok_list);
	(*tok_list) = tok;
	return;
}

void print_tok(token_t *token)
{
	switch(token->type) {
	case TOK_ATOM: printf("ATOM(%s)\n", token->sym); break;
	default: printf("%s\n", token_type_names[token->type]); break;
	}
}
